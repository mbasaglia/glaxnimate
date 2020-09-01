Authors: Mattia Basaglia

# Documentation

The documentation is created with [MkDocs](https://www.mkdocs.org/user-guide/writing-your-docs/).
It lives under `/docs` in the source tree.


## Setup

You need to install some Python dependencies,
it's recommended you do that using `virtualenv`.

To install them, you can run

    make docs_depends_install


## Local development

To run a local server that auto-refreshes when you make changes, run

    make docs_serve


## Building static site

To genrate the HTML run

    make docs

This will generate the files in `docs/site` in the CMake build directory.


## Making changes

Pages use the [Markdown](https://daringfireball.net/projects/markdown/) syntax.

When you create or edit a page make sure you add your name or nickname in
the list of authors at the top of file:

```
Authors: John Doe,
         Alice,
         Bob
```
