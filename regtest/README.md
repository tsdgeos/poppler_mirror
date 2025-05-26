# Regtest

This directory contains the regression testing framework for Poppler. It is designed to detect unintended changes by comparing results against a reference (known good outputs).

## Running the tests

First, you need to create a reference to compare to:

```sh
./poppler-regtest create-refs --refs-dir <REFSDIR> <SRCDIR>
```

It will find all the PDF files in `SRCDIR` and convert them to various output formats, putting the results in `REFSDIR`, including md5 checksums of the results.

To run the tests:
```sh
./poppler-regtest run-tests --create-diffs --out-dir <OUTDIR> --refs-dir <REFSDIR> <SRCDIR>
```

For all the PDFs in `SRCDIR`, it generates fresh outputs and compares them to the refs in `REFSDIR`, placing any non-matching outputs in `OUTDIR` including diff files.

And to create an HTML report in `OUTDIR` based on the test results contained there:
```sh
./poppler-regtest create-report --pretty-diff --no-browser --no-absolute-paths --out-dir <OUTDIR> --refs-dir <OUTDIR> <SRCDIR>: 
```

Run `./poppler-regtest --help` to see all the available commands.
