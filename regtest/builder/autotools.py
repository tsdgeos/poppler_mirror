# autotools.py
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

from builder import Builder, register_builder
import os
import subprocess

class Autotools(Builder):

    def _configure(self):
        autogen = os.path.join(self._srcdir, 'autogen.sh')
        cmd = [autogen, '--prefix=%s' % (self._prefix), '--enable-utils']

        # Disable frontends and tests
        cmd.extend(['--disable-poppler-glib',
                    '--disable-poppler-qt4',
                    '--disable-poppler-cpp',
                    '--disable-gtk-test'])

        backends = self.config.backends
        if backends:
            # Disable backends. Text and ps can't be disabled.
            if 'cairo' not in backends:
                cmd.append('--disable-cairo-output')
            if 'splash' not in backends:
                cmd.append('--disable-splash-output')
        else:
            cmd.extend(['--enable-cairo-output',
                        '--enable-splash-output'])

        p = subprocess.Popen(cmd, cwd=self._srcdir)
        status = p.wait()
        if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
            raise Exception('Command %s returned non-zero exit status %d' % (str(cmd), status))

    def _build(self):
        make = os.environ.get('MAKE', 'make')
        cmd = [make]
        n_cpus = self.number_of_cpus()
        if n_cpus:
            cmd.append('-j%d' % (n_cpus))

        p = subprocess.Popen(cmd, cwd=self._srcdir)
        status = p.wait()
        if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
            raise Exception('Command %s returned non-zero exit status %d' % (str(cmd), status))

register_builder('autotools', Autotools)
