# HTMLReport.py
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

from backends import get_backend, get_all_backends
from Config import Config
import os
import errno
import subprocess

class HTMLPrettyDiff:

    def write(self, test, outdir, actual, expected, diff):
        raise NotImplementedError

    def _create_diff_for_test(self, outdir, test):
        diffdir = os.path.join(outdir, 'html', test)
        try:
            os.makedirs(diffdir)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise
        return diffdir

class HTMLPrettyDiffImage(HTMLPrettyDiff):

    def write(self, test, outdir, result, actual, expected, diff):
        html = """
<html>
<head>
<title>%s</title>
<style>.label{font-weight:bold}</style>
</head>
<body>
Difference between images: <a href="%s">diff</a><br>
<div class=imageText></div>
<div class=imageContainer
     actual="%s"
     expected="%s">Loading...</div>
<script>
(function() {
    var preloadedImageCount = 0;
    function preloadComplete() {
        ++preloadedImageCount;
        if (preloadedImageCount < 2)
            return;
        toggleImages();
        setInterval(toggleImages, 2000)
    }

    function preloadImage(url) {
        image = new Image();
        image.addEventListener('load', preloadComplete);
        image.src = url;
        return image;
    }

    function toggleImages() {
        if (text.textContent == 'Expected Image') {
            text.textContent = 'Actual Image';
            container.replaceChild(actualImage, container.firstChild);
        } else {
            text.textContent = 'Expected Image';
            container.replaceChild(expectedImage, container.firstChild);
        }
    }

    var text = document.querySelector('.imageText');
    var container = document.querySelector('.imageContainer');
    var actualImage = preloadImage(container.getAttribute('actual'));
    var expectedImage = preloadImage(container.getAttribute('expected'));
})();
</script>
</body>
</html>
""" % (test, diff, actual, expected)

        diffdir = self._create_diff_for_test(outdir, test)
        pretty_diff = os.path.abspath(os.path.join(diffdir, result + '-pretty-diff.html'))
        f = open(pretty_diff, 'w')
        f.write(html)
        f.close()

        return pretty_diff

class HTMLPrettyDiffText(HTMLPrettyDiff):
    def write(self, test, outdir, result, actual, expected, diff):
        import difflib

        actual_file = open(actual, 'r')
        expected_file = open(expected, 'r')
        html = difflib.HtmlDiff().make_file(actual_file.readlines(),
                                            expected_file.readlines(),
                                            "Actual", "Expected", context=True)
        actual_file.close()
        expected_file.close()

        diffdir = self._create_diff_for_test(outdir, test)
        pretty_diff = os.path.abspath(os.path.join(diffdir, result + '-pretty-diff.html'))
        f = open(pretty_diff, 'w')
        f.write(html)
        f.close()

        return pretty_diff

def create_pretty_diff(backend):
    if backend.get_diff_ext() == '.diff.png':
        return HTMLPrettyDiffImage()
    if backend.get_diff_ext() == '.diff':
        return HTMLPrettyDiffText()
    return None

class BackendTestResult:

    def __init__(self, test, refsdir, outdir, backend, results):
        self._test = test
        self._refsdir = refsdir
        self._outdir = outdir
        self._backend = backend
        self.config = Config()

        self._results = []

        ref_path = os.path.join(self._refsdir, self._test)
        if not backend.has_md5(ref_path):
            return
        ref_names = backend.get_ref_names(ref_path)
        for result in results:
            basename = os.path.basename(result)
            if basename in ref_names:
                self._results.append(basename)

    def is_failed(self):
        return len(self._results) > 0

    def is_crashed(self):
        return self._backend.is_crashed(os.path.join(self._outdir, self._test))

    def is_failed_to_run(self):
        return self._backend.is_failed(os.path.join(self._outdir, self._test))

    def get_stderr(self):
        return self._backend.get_stderr(os.path.join(self._outdir, self._test))

    def get_failed_html(self):
        html = ""
        for result in self._results:
            actual = os.path.abspath(os.path.join(self._outdir, self._test, result))
            expected = os.path.abspath(os.path.join(self._refsdir, self._test, result))
            html += "<li><a href='%s'>actual</a> <a href='%s'>expected</a> " % (actual, expected)
            if self._backend.has_diff(actual):
                diff = os.path.abspath(actual + self._backend.get_diff_ext())
                html += "<a href='%s'>diff</a> " % (diff)
                if self.config.pretty_diff:
                    pretty_diff = create_pretty_diff(self._backend)
                    if pretty_diff:
                        html += "<a href='%s'>pretty diff</a> " % (pretty_diff.write (self._test, self._outdir, result, actual, expected, diff))
            html += "</li>\n"

        if html:
            return "<ul>%s</ul>\n" % (html)
        return ""


class TestResult:

    def __init__(self, docsdir, refsdir, outdir, resultdir, results, backends):
        self._refsdir = refsdir
        self._outdir = outdir

        self._test = resultdir[len(self._outdir):].lstrip('/')
        self._doc = os.path.join(docsdir, self._test)
        self._results = {}
        for backend in backends:
            ref_path = os.path.join(self._refsdir, self._test)
            if not backend.has_md5(ref_path) and not backend.is_crashed(ref_path) and not backend.is_failed(ref_path):
                continue
            self._results[backend] = BackendTestResult(self._test, refsdir, outdir, backend, results)

    def get_test(self):
        return self._test

    def is_failed(self):
        for backend in self._results:
            if self._results[backend].is_failed():
                return True
        return False;

    def get_failed_html(self):
        html = ""
        for backend in self._results:
            if self._results[backend].is_crashed() or self._results[backend].is_failed_to_run():
                continue
            backend_html = self._results[backend].get_failed_html()
            if not backend_html:
                continue

            html += "<li>%s " % (backend.get_name())
            stderr = self._results[backend].get_stderr()
            if os.path.exists(stderr):
                html += "<a href='%s'>stderr</a>" % (os.path.abspath(stderr))
            html += "</li>\n%s" % (backend_html)

        if html:
            return "<h2><a name='%s'><a href='%s'>%s</a></a></h2>\n<ul>%s</ul><a href='#top'>Top</a>\n" % (self._test, self._doc, self._test, html)
        return ""

    def get_crashed_html(self):
        html = ""
        for backend in self._results:
            if not self._results[backend].is_crashed():
                continue

            html += "<li><a href='%s'>%s</a> (%s)</li>\n" % (self._doc, self._test, backend.get_name())

        if html:
            return "<ul>%s</ul>\n" % (html)
        return ""

    def get_failed_to_run_html(self):
        html = ""
        for backend in self._results:
            status = self._results[backend].is_failed_to_run()
            if not status:
                continue

            html += "<li><a href='%s'>%s</a> [Status: %d] (%s)</li>\n" % (self._doc, self._test, status, backend.get_name())

        if html:
            return "<ul>%s</ul>\n" % (html)
        return ""

class HTMLReport:

    def __init__(self, docsdir, refsdir, outdir):
        self._docsdir = docsdir
        self._refsdir = refsdir
        self._outdir = outdir
        self._htmldir = os.path.join(outdir, 'html')
        self.config = Config()

        try:
            os.makedirs(self._htmldir)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
        except:
            raise

    def create(self):
        html = "<html><body><a name='top'></a>"
        if self.config.backends:
            backends = [get_backend(name) for name in self.config.backends]
        else:
            backends = get_all_backends()

        results = {}
        for root, dirs, files in os.walk(self._outdir, False):
            if not files:
                continue
            if not root.lower().endswith('.pdf'):
                continue
            if root.startswith(self._htmldir):
                continue

            results[root] = TestResult(self._docsdir, self._refsdir, self._outdir, root, files, backends)

        tests = results.keys()
        tests.sort()

        failed_anchors = []
        failed = ""
        crashed = ""
        failed_to_run = ""
        for test_name in tests:
            test = results[test_name]
            if test.is_failed():
                failed_anchors.append(test.get_test())
                failed += test.get_failed_html()
            crashed += test.get_crashed_html()
            failed_to_run += test.get_failed_to_run_html()

        if failed:
            failed = "<h1><a name='failed'>Tests Failed (differences were found)</name></h1>\n%s" % (failed)
        if crashed:
            crashed = "<h1><a name='crashed'>Tests Crashed</a></h1>\n%s" % (crashed)
        if failed_to_run:
            failed_to_run = "<h1><a name='failed_to_run'>Tests that failed to run (command returned an error status)</a></h1>\n%s" % (failed_to_run)

        if failed or crashed or failed_to_run:
            html += "<ul>\n"
            if failed:
                html += "<li><a href='#failed'>Tests Failed (differences were found)</a></li>\n<ul>"
                for anchor in failed_anchors:
                    html += "<li><a href='#%s'>%s</a></li>" % (anchor, anchor)
                html += "</ul>\n"
            if crashed:
                html += "<li><a href='#crashed'>Tests Crashed(differences were found)</a></li>\n"
            if failed_to_run:
                html += "<li><a href='#failed_to_run'>Tests that failed to run (command returned an error status)</a></li>\n"
            html += "</ul>\n"

        html += failed + crashed + failed_to_run + "</body></html>"

        report_index = os.path.join(self._htmldir, 'index.html')
        f = open(report_index, 'wb')
        f.write(html)
        f.close()

        subprocess.Popen(['xdg-open', report_index])
