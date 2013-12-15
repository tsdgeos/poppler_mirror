# create-report.py
#
# Copyright (C) 2012 Carlos Garcia Campos <carlosgc@gnome.org>
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

from commands import Command, register_command
from HTMLReport import HTMLReport
from Config import Config
import os
import tempfile

class CreateReport(Command):

    name = 'create-report'
    usage_args = '[ options ... ] tests '
    description = 'Create report of test results'

    def __init__(self):
        Command.__init__(self)
        parser = self._get_args_parser()
        parser.add_argument('--refs-dir',
                            action = 'store', dest = 'refs_dir', default = os.path.join(tempfile.gettempdir(), 'refs'),
                            help = 'Directory containing the references')
        parser.add_argument('-o', '--out-dir',
                            action = 'store', dest = 'out_dir', default = os.path.join(tempfile.gettempdir(), 'out'),
                            help = 'Directory containing the results')
        parser.add_argument('-p', '--pretty-diff',
                            action = 'store_true', dest = 'pretty_diff', default = False,
                            help = 'Include pretty diff output')
        parser.add_argument('tests')

    def run(self, options):
        config = Config()
        config.pretty_diff = options['pretty_diff']

        doc = options['tests']
        if os.path.isdir(doc):
            docs_dir = doc
        else:
            docs_dir = os.path.dirname(doc)

        report = HTMLReport(docs_dir, options['refs_dir'], options['out_dir'])
        report.create()

        return 0

register_command('create-report', CreateReport)
