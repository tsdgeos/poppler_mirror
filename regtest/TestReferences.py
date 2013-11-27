# TestReferences.py
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

import os
import errno
from backends import get_backend, get_all_backends
from Config import Config
from Printer import get_printer
from Utils import get_document_paths_from_dir, get_skipped_tests

from Queue import Queue
from threading import Thread, RLock

class TestReferences:

    def __init__(self, docsdir, refsdir):
        self._docsdir = docsdir
        self._refsdir = refsdir
        self._skipped = get_skipped_tests(docsdir)
        self.config = Config()
        self.printer = get_printer()
        self._total_tests = 1
        self._n_tests = 0

        self._queue = Queue()
        self._lock = RLock()

        try:
            os.makedirs(self._refsdir)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise

    def _get_backends(self):
        if self.config.backends:
            return [get_backend(name) for name in self.config.backends]

        return get_all_backends()

    def create_refs_for_file(self, filename):
        backends = self._get_backends()

        if filename in self._skipped:
            with self._lock:
                self._n_tests += len(backends)
            self.printer.print_default("Skipping test '%s'" % (os.path.join(self._docsdir, filename)))
            return

        refs_path = os.path.join(self._refsdir, filename)
        try:
            os.makedirs(refs_path)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise
        doc_path = os.path.join(self._docsdir, filename)

        for backend in backends:
            if not self.config.force and backend.has_results(refs_path):
                with self._lock:
                    self._n_tests += 1
                self.printer.print_default("Results found, skipping '%s' for %s backend" % (doc_path, backend.get_name()))
                continue

            if backend.create_refs(doc_path, refs_path):
                backend.create_checksums(refs_path, self.config.checksums_only)
            with self._lock:
                self._n_tests += 1
                self.printer.printout_ln("[%d/%d] %s (%s): done" % (self._n_tests, self._total_tests, doc_path, backend.get_name()))

    def _worker_thread(self):
        while True:
            doc = self._queue.get()
            self.create_refs_for_file(doc)
            self._queue.task_done()

    def create_refs(self):
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
