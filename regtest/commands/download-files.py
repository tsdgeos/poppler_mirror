# download-files.py
#
# Copyright (C) 2022 Albert Astals Cid <aacid@kde.org>
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
from __future__ import absolute_import, division, print_function

from commands import Command, register_command
from Timer import Timer
from Printer import get_printer
import os
import tempfile
import urllib.request

class DownloadFiles(Command):

    name = 'download-files'
    usage_args = '[ options ... ] file-with-files '
    description = 'Downloads files listed in the given file. For each file the first word is the output filename and the second is the url to download from'

    def __init__(self):
        Command.__init__(self)
        parser = self._get_args_parser()
        parser.add_argument('--files-dir',
                            action = 'store', dest = 'files_dir', default = os.path.join(tempfile.gettempdir(), 'files'),
                            help = 'Directory where the files will be downloaded')
        parser.add_argument('-f', '--force',
                            action = 'store_true', dest = 'force', default = False,
                            help = 'Create references again for tests that already have references')
        parser.add_argument('file-with-files')

    def run(self, options):
        t = Timer()
        filepath = options['file-with-files']

        files_dir = options['files_dir']
        os.makedirs(files_dir, exist_ok=True)

        f = open(filepath, "r")
        for line in f:
            splitted_line = line.split()
            if not len(splitted_line) == 2:
                get_printer().printout_ln("Malformed line %s" % line)
                return 1

            target_path = os.path.join(files_dir, splitted_line[0]);
            if os.path.exists(target_path) and not options['force']:
                get_printer().printout_ln("Skipping %s already exists" % splitted_line[0])
            else:
                get_printer().printout_ln("Downloading %s to %s" % (splitted_line[1] , target_path))
                urllib.request.urlretrieve(splitted_line[1], target_path)

        get_printer().printout_ln("Files downloaded in %s" % (t.elapsed_str()))

        return 0

register_command('download-files', DownloadFiles)
