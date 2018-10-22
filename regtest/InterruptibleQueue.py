# InterruptibleQueue.py
#
# Copyright (C) 2016 Carlos Garcia Campos <carlosgc@gnome.org>
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
from __future__ import absolute_import, print_function

from threading import Lock, Condition
from collections import deque
import sys

class InterruptibleQueue:
    """Simpler implementation of Queue that uses wait with a timeout to make join interruptile"""

    def __init__(self):
        self._queue = deque()
        self._mutex = Lock()
        self._finished_condition = Condition(self._mutex)
        self._not_empty_condition = Condition(self._mutex)
        self._n_unfinished_tasks = 0

    def task_done(self):
        self._finished_condition.acquire()
        try:
            n_unfinished = self._n_unfinished_tasks - 1
            if n_unfinished == 0:
                self._finished_condition.notify_all()
            self._n_unfinished_tasks = n_unfinished
        finally:
            self._finished_condition.release()

    def join(self):
        self._finished_condition.acquire()
        try:
            while self._n_unfinished_tasks:
                self._finished_condition.wait(sys.float_info.max)
        finally:
            self._finished_condition.release()

    def put(self, item):
        self._mutex.acquire()
        try:
            self._queue.append(item)
            self._n_unfinished_tasks += 1
            self._not_empty_condition.notify()
        finally:
            self._mutex.release()

    def get(self):
        self._not_empty_condition.acquire()
        try:
            while not len(self._queue):
                self._not_empty_condition.wait()
            return self._queue.popleft()
        finally:
            self._not_empty_condition.release()

