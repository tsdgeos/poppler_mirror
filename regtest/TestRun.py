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
        self._failed = []
        self._crashed = []
        self._failed_status_error = []
        self._stderr = []
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

        with self._lock:
            self._n_tests += 1
            self._n_run += 1

            if backend.has_stderr(test_path):
                self._stderr.append("%s (%s)" % (doc_path, backend.get_name()))

            if ref_has_md5 and test_has_md5:
                if test_passed:
                    # FIXME: remove dir if it's empty?
                    self.printer.print_test_result(doc_path, backend.get_name(), self._n_tests, self._total_tests, "PASS")
                    self._n_passed += 1
                else:
                    self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "FAIL")
                    self._failed.append("%s (%s)" % (doc_path, backend.get_name()))
                return

            if test_has_md5:
                if ref_is_crashed:
                    self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "DOES NOT CRASH")
                elif ref_is_failed:
                    self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "DOES NOT FAIL")
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
                self._crashed.append("%s (%s)" % (doc_path, backend.get_name()))
                return

            if test_is_failed:
                self.printer.print_test_result_ln(doc_path, backend.get_name(), self._n_tests, self._total_tests, "FAIL (status error %d)" % (test_is_failed))
                self._failed_status_error("%s (%s)" % (doc_path, backend.get_name()))
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

    def run_tests(self):
        docs, total_docs = get_document_paths_from_dir(self._docsdir)
        backends = self._get_backends()
        self._total_tests = total_docs * len(backends)

        self.printer.printout_ln('Found %d documents' % (total_docs))
        self.printer.printout_ln('Backends: %s' % ', '.join([backend.get_name() for backend in backends]))
        self.printer.printout_ln('Process %d using %d worker threads' % (os.getpid(), self.config.threads))
        self.printer.printout_ln()

        self.printer.printout('Spawning %d workers...' % (self.config.threads))

        for n_thread in range(self.config.threads):
            thread = Thread(target=self._worker_thread)
            thread.daemon = True
            thread.start()

        for doc in docs:
            self._queue.put(doc)

        self._queue.join()

    def summary(self):
        self.printer.printout_ln()

        if self._n_run:
            self.printer.printout_ln("%d tests passed (%.2f%%)" % (self._n_passed, (self._n_passed * 100.) / self._n_run))
            self.printer.printout_ln()
            def report_tests(test_list, test_type):
                n_tests = len(test_list)
                if not n_tests:
                    return
                self.printer.printout_ln("%d tests %s (%.2f%%): %s" % (n_tests, test_type, (n_tests * 100.) / self._n_run, ", ".join(test_list)))
                self.printer.printout_ln()

            report_tests(self._failed, "failed")
            report_tests(self._crashed, "crashed")
            report_tests(self._failed_status_error, "failed to run")
            report_tests(self._stderr, "have stderr output")
        else:
            self.printer.printout_ln("No tests run")

        if self._skipped:
            self.printer.printout_ln("%d tests skipped: %s" % (len(self._skipped), ", ".join(self._skipped)))
            self.printer.printout_ln()

        if self._new:
            self.printer.printout_ln("%d new documents: %s\nUse create-refs command to add reference results for them" % (len(self._new), ", ".join(self._new)))
            self.printer.printout_ln()
