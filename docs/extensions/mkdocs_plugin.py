import urllib.parse
from mkdocs.utils import normalize_url

try:
    from jinja2 import pass_context as contextfilter
except ImportError:
    from jinja2 import contextfilter

@contextfilter
def canonical_url(context, value):
    return urllib.parse.urljoin(context["page"].canonical_url, value)


def on_env(env, **kw):
    env.filters["canonical_url"] = canonical_url
