# find-regression.py
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
from Bisect import Bisect
from Timer import Timer
from Config import Config
from Printer import get_printer
import os
import tempfile

class FindRegression(Command):

    name = 'find-regression'
    usage_args = '[ options ... ] test '
    description = 'Find revision that introduced a regression for the given test'

    def __init__(self):
        Command.__init__(self)
        parser = self._get_args_parser()
        parser.add_argument('--refs-dir',
                            action = 'store', dest = 'refs_dir', default = os.path.join(tempfile.gettempdir(), 'refs'),
                            help = 'Directory containing the references')
        parser.add_argument('-o', '--out-dir',
                            action = 'store', dest = 'out_dir', default = os.path.join(tempfile.gettempdir(), 'out'),
                            help = 'Directory containing the results')
        parser.add_argument('--src-dir',
                            action = 'store', dest = 'src_dir', default = os.path.abspath("../"),
                            help = 'Directory of poppler sources')
        parser.add_argument('--builder',
                            action = 'store', dest = 'builder',
                            choices=['autotools'], default = 'autotools',
                            help = 'Build system to use')
        parser.add_argument('--prefix',
                            action = 'store', dest = 'prefix', default = '/usr/local',
                            help = 'Build prefix')
        parser.add_argument('--good',
                            action = 'store', dest = 'good', metavar = 'REVISION',
                            help = 'Good revision')
        parser.add_argument('--bad',
                            action = 'store', dest = 'bad', default = 'HEAD', metavar = 'REVISION',
                            help = 'Bad revision')
        parser.add_argument('test')

    def run(self, options):
        config = Config()
        config.src_dir = options['src_dir']
        config.builder = options['builder']
        config.prefix = options['prefix']
        config.good = options['good']
        config.bad = options['bad']

        doc = options['test']
        if not os.path.isfile(doc):
            get_printer().printerr("Invalid test %s: not a regulat file" % (doc))
            return 1

        t = Timer()
        bisect = Bisect(options['test'], options['refs_dir'], options['out_dir'])
        bisect.run()
        get_printer().printout_ln("Tests run in %s" % (t.elapsed_str()))

        return 0

register_command('find-regression', FindRegression)
