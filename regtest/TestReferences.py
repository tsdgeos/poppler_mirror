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

class TestReferences:

    def __init__(self, docsdir, refsdir):
        self._docsdir = docsdir
        self._refsdir = refsdir
        self.config = Config()

        try:
            os.makedirs(self._refsdir)
        except OSError, e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise

    def create_refs_for_file(self, filename):
        refs_path = os.path.join(self._refsdir, filename)
        try:
            os.makedirs(refs_path)
        except OSError, e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise
        doc_path = os.path.join(self._docsdir, filename)

        if self.config.backends:
            backends = [get_backend(name) for name in self.config.backends]
        else:
            backends = get_all_backends()

        for backend in backends:
            if not self.config.force and backend.has_md5(refs_path):
                print "Checksum file found, skipping '%s' for %s backend" % (doc_path, backend.get_name())
                continue
            print "Creating refs for '%s' using %s backend" % (doc_path, backend.get_name())
            if backend.create_refs(doc_path, refs_path):
                backend.create_checksums(refs_path, self.config.checksums_only)

    def create_refs(self):
        for root, dirs, files in os.walk(self._docsdir, False):
            for entry in files:
                if not entry.lower().endswith('.pdf'):
                    continue

                test_path = os.path.join(root[len(self._docsdir):], entry)
                self.create_refs_for_file(test_path.lstrip(os.path.sep))



