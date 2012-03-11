# builder
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

from Config import Config
import os
import sys
import subprocess

__all__ = [ 'register_builder',
            'get_builder',
            'UnknownBuilderError',
            'Builder' ]

class UnknownBuilderError(Exception):
    '''Unknown builder'''

class Builder:

    def __init__(self):
        self.config = Config()
        self._srcdir = self.config.src_dir
        self._prefix = self.config.prefix

    def number_of_cpus(self):
        if not sys.platform.startswith("linux"):
            # TODO
            return 0

        n_cpus = subprocess.Popen(['nproc'], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
        if n_cpus:
            return int(n_cpus)

        n_cpus = 0
        f = open('/proc/cpuinfo', 'r')
        for line in f.readlines():
            if 'processor' in line:
                n_cpus += 1
        f.close()

        return n_cpus

    def _configure(self):
        raise NotImplementedError

    def _build(self):
        raise NotImplementedError

    def build(self):
        self._configure()
        self._build()


_builders = {}
def register_builder(builder_name, builder_class):
    _builders[builder_name] = builder_class

def _get_builder(builder_name):
    if builder_name not in _builders:
        try:
            __import__('builder.%s' % builder_name)
        except ImportError:
            pass

    if builder_name not in _builders:
        raise UnknownBuilderError('Invalid %s builder' % builder_name)

    return _builders[builder_name]

def get_builder(builder_name):
    builder_class = _get_builder(builder_name)
    return builder_class()
