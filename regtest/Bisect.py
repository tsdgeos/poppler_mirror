# Bisect.py
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

from builder import get_builder
from TestRun import TestRun
from Config import Config
import os
import subprocess
import sys

class GitBisect:
    def __init__(self, srcdir):
        self.srcdir = srcdir

    def __run_cmd(self, cmd):
        p = subprocess.Popen(cmd, cwd=self.srcdir, stdout=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if stdout:
            sys.stdout.write(stdout)

        status = p.returncode
        if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
            raise Exception('Command %s returned non-zero exit status %d' % (str(cmd), status))

        return stdout

    def __finished(self, output):
        if not output:
            return True

        return "is the first bad commit" in output

    def start(self, bad=None, good=None):
        cmd = ['git', 'bisect', 'start']
        if bad is not None:
            cmd.append(bad)
            if good is not None:
                cmd.append(good)
        self.__run_cmd(cmd)

    def good(self):
        cmd = ['git', 'bisect', 'good']
        output = self.__run_cmd(cmd)
        return self.__finished(output)

    def bad(self):
        cmd = ['git', 'bisect', 'bad']
        output = self.__run_cmd(cmd)
        return self.__finished(output)

    def reset(self):
        cmd = ['git', 'bisect', 'reset']
        self.__run_cmd(cmd)

class Bisect:

    def __init__(self, test, refsdir, outdir):
        self._test = test
        self._refsdir = refsdir
        self._outdir = outdir
        self.config = Config()
        self._builder = get_builder(self.config.builder)

        # Add run-tests options to config
        self.config.keep_results = False
        self.config.create_diffs = False
        self.config.update_refs = False

    def __get_current_revision(self):
        p = subprocess.Popen(['git', 'rev-parse', 'HEAD'], cwd=self.config.src_dir, stdout=subprocess.PIPE)
        return p.communicate()[0]

    def run(self):
        bisect = GitBisect(self.config.src_dir)
        # TODO: save revision in .md5 files and get the good
        # revision from refs when not provided by command line
        try:
            bisect.start(self.config.bad, self.config.good)
        except:
            print("Couldn't start git bisect")
            return
        finished = False
        while not finished:
            test_runner = TestRun(os.path.dirname(self._test), self._refsdir, self._outdir)
            try:
                self._builder.build()
            except:
                print("Impossible to find regression, build is broken in revision: %s" % (self.__get_current_revision()))
                break
            test_runner.run_test(os.path.basename(self._test))
            if test_runner._n_passed == 0:
                finished = bisect.bad()
            else:
                finished = bisect.good()

        bisect.reset()

