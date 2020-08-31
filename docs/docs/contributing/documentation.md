Authors: Mattia Basaglia

# Documentation

The documentation is created with [MkDocs](https://www.mkdocs.org/user-guide/writing-your-docs/).
It lives under `/docs` in the source tree.

## Setup

You need to install `mkdocs`

    pip install mkdocs
    
    
## Local development

To run a local server that auto-refreshes when you make changes, run

    mkdocs serve
    
from the `docs` directory.


## Building static site

To genrate the HTML run

    mkdocs build
    
This will generate the files in `docs/site`.


## Making changes

Pages use the [Markdown](https://daringfireball.net/projects/markdown/) syntax.

When you create or edit a page make sure you add your name or nickname in
the list of authors, [as shown here](https://www.mkdocs.org/user-guide/writing-your-docs/#multimarkdown-style-meta-data).
