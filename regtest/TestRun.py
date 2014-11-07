# TestRun.py
#
# Copyright (C) 2011 Carlos Garcia Campos <carlosgc@gnome.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

from backends import get_backend, get_all_backends
from Config import Config
from Utils import get_document_paths_from_dir, get_skipped_tests
from Printer import get_printer
import sys
import os
import errno

from Queue import Queue
from threading import Thread, RLock

class TestRun:

    def __init__(self, docsdir, refsdir, outdir):
        self._docsdir = docsdir
        self._refsdir = refsdir
        self._outdir = outdir
        self._skip = get_skipped_tests(docsdir)
        self.config = Config()
        self.printer = get_printer()
        self._total_tests = 1

        # Results
        self._n_tests = 0
        self._n_run = 0
        self._n_passed = 0
        self._failed = {}
        self._crashed = {}
        self._failed_status_error = {}
        self._did_not_crash = {}
        self._did_not_fail_status_error = {}
        self._stderr = {}
        self._skipped = []
        self._new = []

        self._queue = Queue()
        self._lock = RLock()

        try:
            os.makedirs(self._outdir);
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise

    def _get_backends(self):
        if self.config.backends:
            return [get_backend(name) for name in self.config.backends]

        return get_all_backends()

    def test(self, refs_path, doc_path, test_path, backend):
        # First check whether there are test results for the backend
        ref_has_md5 = backend.has_md5(refs_path)
        ref_is_crashed = backend.is_crashed(refs_path)
        ref_is_failed = backend.is_failed(refs_path)
        if not ref_has_md5 and not ref_is_crashed and not ref_is_failed:
            with self._lock:
                self._new.append("%s (%s)" % (doc_path, backend.get_name()))
                self._n_tests += 1
            self.printer.print_default("Reference files not found, skipping '%s' for %s backend" % (doc_path, backend.get_name()))
            return

        test_has_md5 = backend.create_refs(doc_path, test_path)
        test_passed = False
        if ref_has_md5 and test_has_md5:
            test_passed = backend.compare_checksums(refs_path, test_path, not self.config.keep_results, self.config.create_diffs, self.config.update_refs)
        elif self.config.update_refs:
            backend.update_results(refs_path, test_path)

        with self._lock:
            self._n_tests += 1
            self._n_run += 1

            if backend.has_stderr(test_path):
                self._stderr.setdefault(backend.get_name(), []).append(doc_path)

            if ref_has_md5 and test_has_md5:
                if test_passed:
                    # FIXME: remove dir if it's empty?
                    self.printer.print_test_result(doc_path, backend.get_name(), self._n_tests, self._total_tests, "PASS")
                    self._n_passed += 1
                else:
                    self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "FAIL")
                    self._failed.setdefault(backend.get_name(), []).append(doc_path)
                return

            if test_has_md5:
                if ref_is_crashed:
                    self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "DOES NOT CRASH")
                    self._did_not_crash.setdefault(backend.get_name(), []).append(doc_path)
                elif ref_is_failed:
                    self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "DOES NOT FAIL")
                    self._did_not_fail_status_error.setdefault(backend.get_name(), []).append(doc_path)
                return

            test_is_crashed = backend.is_crashed(test_path)
            if ref_is_crashed and test_is_crashed:
                self.printer.print_test_result(doc_path, backend.get_name(), self._n_tests, self._total_tests, "PASS (Expected crash)")
                self._n_passed += 1
                return

            test_is_failed = backend.is_failed(test_path)
            if ref_is_failed and test_is_failed:
                # FIXME: compare status errors
                self.printer.print_test_result(doc_path, backend.get_name(), self._n_tests, self._total_tests, "PASS (Expected fail with status error %d)" % (test_is_failed))
                self._n_passed += 1
                return

            if test_is_crashed:
                self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "CRASH")
                self._crashed.setdefault(backend.get_name(), []).append(doc_path)
                return

            if test_is_failed:
                self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "FAIL (status error %d)" % (test_is_failed))
                self._failed_status_error.setdefault(backend.get_name(), []).append(doc_path)
                return

    def run_test(self, filename):
        backends = self._get_backends()

        if filename in self._skip:
            doc_path = os.path.join(self._docsdir, filename)
            with self._lock:
                self._skipped.append("%s" % (doc_path))
                self._n_tests += len(backends)
            self.printer.print_default("Skipping test '%s'" % (doc_path))
            return

        out_path = os.path.join(self._outdir, filename)
        try:
            os.makedirs(out_path)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise
        doc_path = os.path.join(self._docsdir, filename)
        refs_path = os.path.join(self._refsdir, filename)

        if not os.path.isdir(refs_path):
            with self._lock:
                self._new.append("%s" % (doc_path))
                self._n_tests += len(backends)
            self.printer.print_default("Reference dir not found for %s, skipping" % (doc_path))
            return

        for backend in backends:
            self.test(refs_path, doc_path, out_path, backend)

    def _worker_thread(self):
        while True:
            doc = self._queue.get()
            self.run_test(doc)
            self._queue.task_done()

    def run_tests(self, tests = []):
        if not tests:
            docs, total_docs = get_document_paths_from_dir(self._docsdir)
        else:
            docs = []
            total_docs = 0
            for test in tests:
                if os.path.isdir(test):
                    test_dir = test
                elif os.path.isdir(os.path.join(self._docsdir, test)):
                    test_dir = os.path.join(self._docsdir, test)
                else:
                    test_dir = None

                if test_dir is not None:
                    dir_docs, dir_n_docs = get_document_paths_from_dir(test_dir, self._docsdir)
                    docs.extend(dir_docs)
                    total_docs += dir_n_docs
                else:
                    if test.startswith(self._docsdir):
                        test = test[len(self._docsdir):].lstrip(os.path.sep)
                    docs.append(test)
                    total_docs += 1

        backends = self._get_backends()
        self._total_tests = total_docs * len(backends)

        if total_docs == 1:
            n_workers = 0
        else:
            n_workers = min(self.config.threads, total_docs)

        self.printer.printout_ln('Found %d documents' % (total_docs))
        self.printer.printout_ln('Backends: %s' % ', '.join([backend.get_name() for backend in backends]))
        self.printer.printout_ln('Process %d using %d worker threads' % (os.getpid(), n_workers))
        self.printer.printout_ln()

        if n_workers > 0:
            self.printer.printout('Spawning %d workers...' % (self.config.threads))

            for n_thread in range(n_workers):
                thread = Thread(target=self._worker_thread)
                thread.daemon = True
                thread.start()

            for doc in docs:
                self._queue.put(doc)

            self._queue.join()
        else:
            for doc in docs:
                self.run_test(doc)

        return int(self._n_passed != self._n_run)

    def summary(self):
        self.printer.printout_ln()

        if self._n_run:
            self.printer.printout_ln("%d tests passed (%.2f%%)" % (self._n_passed, (self._n_passed * 100.) / self._n_run))
            self.printer.printout_ln()

            def result_tests(test_dict):
                if not test_dict:
                    return 0, None

                n_tests = 0
                tests = ""
                for backend in test_dict:
                    backend_docs = test_dict[backend]
                    n_tests += len(backend_docs)
                    tests += "\n".join(["  %s (%s)" % (doc_path, backend) for doc_path in backend_docs])
                    tests += "\n"

                return n_tests, tests

            def backends_summary(test_dict, n_tests):
                percs = []
                for backend in test_dict:
                    n_docs = len(test_dict[backend])
                    percs.append("%d %s (%.2f%%)" % (n_docs, backend, (n_docs * 100.) / n_tests))
                return ", ".join(percs)

            test_results = [(self._failed, "unexpected failures"),
                            (self._crashed, "unexpected crashes"),
                            (self._failed_status_error, "unexpected failures (test program returned with an exit error status)"),
                            (self._stderr, "tests have stderr output"),
                            (self._did_not_crash, "expected to crash, but didn't crash"),
                            (self._did_not_fail_status_error, "expected to fail to run, but didn't fail")]

            for test_dict, test_msg in test_results:
                n_tests, tests = result_tests(test_dict)
                if n_tests == 0:
                    continue

                self.printer.printout_ln("%d %s (%.2f%%) [%s]" % (n_tests, test_msg, (n_tests * 100.) / self._n_run, backends_summary(test_dict, n_tests)))
                self.printer.printout_ln(tests)
                self.printer.printout_ln()
        else:
            self.printer.printout_ln("No tests run")

        if self._skipped:
            self.printer.printout_ln("%d tests skipped" % len(self._skipped))
            self.printer.printout_ln("\n".join(["  %s" % skipped for skipped in self._skipped]))
            self.printer.printout_ln()

        if self._new:
            self.printer.printout_ln("%d new documents" % len(self._new))
            self.printer.printout_ln("\n".join(["  %s" % new for new in self._new]))
            self.printer.printout_ln("Use create-refs command to add reference results for them")
            self.printer.printout_ln()
