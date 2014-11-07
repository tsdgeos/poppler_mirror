# backends
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

import hashlib
import os
import select
import shutil
import errno
from Config import Config
from Printer import get_printer

__all__ = [ 'register_backend',
            'get_backend',
            'get_all_backends',
            'UnknownBackendError',
            'Backend' ]

class UnknownBackendError(Exception):
    '''Unknown backend type'''

class Backend:

    def __init__(self, name, diff_ext = None):
        self._name = name
        self._diff_ext = diff_ext
        self._utilsdir = Config().utils_dir

        self.printer = get_printer()

    def get_name(self):
        return self._name

    def get_diff_ext(self):
        return self._diff_ext

    def __md5sum(self, ref_path):
        md5 = hashlib.md5()
        with open(ref_path,'rb') as f:
            for chunk in iter(lambda: f.read(128 * md5.block_size), b''):
                md5.update(chunk)

        return md5.hexdigest()

    def __should_have_checksum(self, entry):
        if not entry.startswith(self._name):
            return False

        name, ext = os.path.splitext(entry)
        return ext not in ('.md5', '.crashed', '.failed', '.stderr');

    def create_checksums(self, refs_path, delete_refs = False):
        path = os.path.join(refs_path, self._name)
        md5_file = open(path + '.md5', 'w')

        for entry in sorted(os.listdir(refs_path)):
            if not self.__should_have_checksum(entry):
                continue
            ref_path = os.path.join(refs_path, entry)
            md5_file.write("%s %s\n" % (self.__md5sum(ref_path), ref_path))
            if delete_refs:
                os.remove(ref_path)

        md5_file.close()

    def compare_checksums(self, refs_path, out_path, remove_results = True, create_diffs = True, update_refs = False):
        retval = True

        md5_path = os.path.join(refs_path, self._name)
        md5_file = open(md5_path + '.md5', 'r')
        tests = os.listdir(out_path)
        result_md5 = []

        for line in md5_file.readlines():
            md5sum, ref_path = line.strip('\n').split(' ', 1)
            basename = os.path.basename(ref_path)
            if not self.__should_have_checksum(basename):
                continue

            if not basename in tests:
                retval = False
                self.printer.print_default("%s found in md5 ref file but missing in output dir %s" % (basename, out_path))
                continue

            result_path = os.path.join(out_path, basename)

            result_md5sum = self.__md5sum(result_path);
            matched = md5sum == result_md5sum

            if update_refs:
                result_md5.append("%s %s\n" % (result_md5sum, ref_path))

            if matched:
                if remove_results:
                    os.remove(result_path)
            else:
                self.printer.print_default("Differences found in %s" % (basename))
                if create_diffs:
                    if not os.path.exists(ref_path):
                        self.printer.print_default("Reference file %s not found, skipping diff for %s" % (ref_path, result_path))
                    else:
                        try:
                            self._create_diff(ref_path, result_path)
                        except NotImplementedError:
                            # Diff not supported by backend
                            pass

                if update_refs:
                    if os.path.exists(ref_path):
                        self.printer.print_default("Updating image reference %s" % (ref_path))
                        shutil.copyfile(result_path, ref_path)

                retval = False
        md5_file.close()

        if update_refs and not retval:
            self.printer.print_default("Updating md5 reference %s" % (md5_path))
            f = open(md5_path + '.md5.tmp', 'wb')
            f.writelines(result_md5)
            f.close()
            os.rename(md5_path + '.md5.tmp', md5_path + '.md5')

            for ref in ('.crashed', '.failed', '.stderr'):
                src = os.path.join(out_path, self._name + ref)
                dest = os.path.join(refs_path, self._name + ref)
                try:
                    shutil.copyfile(src, dest)
                except IOError as e:
                    if e.errno != errno.ENOENT:
                        raise

        return retval

    def update_results(self, refs_path, out_path):
        if not self.has_md5(refs_path):
            path = os.path.join(refs_path, self._name)
            md5_file = open(path + '.md5', 'w')

            for entry in sorted(os.listdir(out_path)):
                if not self.__should_have_checksum(entry):
                    continue
                result_path = os.path.join(out_path, entry)
                ref_path = os.path.join(refs_path, entry)
                md5_file.write("%s %s\n" % (self.__md5sum(result_path), ref_path))
                shutil.copyfile(result_path, ref_path)

            md5_file.close()

        for ref in ('.crashed', '.failed', '.stderr'):
            result_path = os.path.join(out_path, self._name + ref)
            ref_path = os.path.join(refs_path, self._name + ref)

            if os.path.exists(result_path):
                shutil.copyfile(result_path, ref_path)
            elif os.path.exists(ref_path):
                os.remove(ref_path)

    def get_ref_names(self, refs_path):
        retval = []
        md5_path = os.path.join(refs_path, self._name)
        md5_file = open(md5_path + '.md5', 'r')
        for line in md5_file.readlines():
            md5sum, ref_path = line.strip('\n').split(' ', 1)
            basename = os.path.basename(ref_path)
            if not self.__should_have_checksum(basename):
                continue

            retval.append(basename)
        md5_file.close()

        return retval

    def has_md5(self, test_path):
        return os.path.exists(os.path.join(test_path, self._name + '.md5'))

    def is_crashed(self, test_path):
        return os.path.exists(os.path.join(test_path, self._name + '.crashed'))

    def is_failed(self, test_path):
        failed_path = os.path.join(test_path, self._name + '.failed')
        if not os.path.exists(failed_path):
            return 0

        f = open(failed_path, 'r')
        status = int(f.read())
        f.close()

        return status

    def has_results(self, test_path):
        return self.has_md5(test_path) or self.is_crashed(test_path) or self.is_failed(test_path)

    def get_stderr(self, test_path):
        return os.path.join(test_path, self._name + '.stderr')

    def has_stderr(self, test_path):
        return os.path.exists(self.get_stderr(test_path))

    def has_diff(self, test_result):
        if not self._diff_ext:
            return False
        basename = os.path.basename(test_result)
        if not basename.startswith(self._name):
            return False
        return os.path.exists(test_result + self._diff_ext)

    def __create_failed_file_if_needed(self, status, out_path):
        if os.WIFEXITED(status) or os.WEXITSTATUS(status) == 0:
            return False

        failed_file = open(out_path + '.failed', 'w')
        failed_file.write("%d" % (os.WEXITSTATUS(status)))
        failed_file.close()

        return True

    def __redirect_stderr_to_file(self, fd, out_path):
        stderr_file = None
        max_size = 1024 * 1024
        read_set = [fd]

        while read_set:
            try:
                rlist, wlist, xlist = select.select(read_set, [], [])
            except select.error as e:
                continue

            if fd in rlist:
                try:
                    chunk = os.read(fd, 1024)
                except OSError as e:
                    if e.errno == errno.EIO:
                        # Child process finished.
                        chunk = ''
                    else:
                        raise e
                if chunk:
                    if stderr_file is None:
                        stderr_file = open(out_path + '.stderr', 'wb')
                    if max_size > 0:
                        stderr_file.write(chunk)
                        max_size -= len(chunk)
                else:
                    read_set.remove(fd)

        if stderr_file is not None:
            stderr_file.close()

    def _check_exit_status(self, p, out_path):
        self.__redirect_stderr_to_file(p.stderr.fileno(), out_path)
        status = p.wait()

        if not os.WIFEXITED(status):
            open(out_path + '.crashed', 'w').close()
            return False

        if self.__create_failed_file_if_needed(status, out_path):
            return False

        return True

    def _diff_png(self, ref_path, result_path):
        try:
            from PIL import Image, ImageChops
        except ImportError:
            raise NotImplementedError

        ref = Image.open(ref_path)
        result = Image.open(result_path)
        diff = ImageChops.difference(ref, result)
        diff.save(result_path + '.diff.png', 'png')

    def _create_diff(self, ref_path, result_path):
        raise NotImplementedError

    def create_refs(self, doc_path, refs_path):
        raise NotImplementedError

_backends = {}
def register_backend(backend_name, backend_class):
    _backends[backend_name] = backend_class

def _get_backend(backend_name):
    if backend_name not in _backends:
        try:
            __import__('backends.%s' % backend_name)
        except ImportError:
            pass

    if backend_name not in _backends:
        raise UnknownBackendError('Backend %s does not exist' % backend_name)

    return _backends[backend_name]

def get_backend(backend_name):
    backend_class = _get_backend(backend_name)
    return backend_class(backend_name)

def get_all_backends():
    backends = []

    thisdir = os.path.abspath(os.path.dirname(__file__))
    for fname in os.listdir(os.path.join(thisdir)):
        name, ext = os.path.splitext(fname)
        if not ext == '.py':
            continue
        try:
            __import__('backends.%s' % name)
        except ImportError:
            continue

        if name in _backends:
            backends.append(_backends[name](name))

    return backends
