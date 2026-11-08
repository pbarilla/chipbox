"""The one menu every Chipbox page shares: the same eight pills as the home page, in the same order, the current page pressed in.
Pages are made by build/gen_*.py, tools/make_cheatsheet.py and by hand; they all call fix() (or paste gbnav()), so the menu cannot drift."""
import re

REPO = "https://github.com/pbarilla/chipbox"
ITEMS = [("install", "install.html", "Install", "hot"), ("play", "play/", "Play online", ""), ("tutorials", "manual/", "Tutorials", ""), ("manual", "guide/", "Manual", ""),
         ("projects", "projects/", "Projects", ""), ("cheatsheet", "cheatsheet/", "Cheat sheet", "")]


def gbnav(rel, here=None):
    """rel: the path up to the site's root ('', '../', '../../'); here: the key of the page it is on"""
    out = []
    for key, href, name, cls in ITEMS:
        a = ' aria-current="page"' if key == here else ""
        out.append('<a%s href="%s%s"%s>%s</a>' % (' class="%s"' % cls if cls else "", rel, href, a, name))
    out.append('<a href="%s/releases/latest">Download</a>' % REPO)
    out.append('<a href="%s">Source</a>' % REPO)
    return '<nav class="gbnav" aria-label="Pages">%s</nav>' % "".join(out)


def fix(page, rel, here=None):
    """take out every old menu and put the shared one under the heading (under the header, when the heading sits in one)"""
    if "favicon.svg" not in page:
        page = page.replace("</head>", '<link rel="icon" type="image/svg+xml" href="%sfavicon.svg">\n</head>' % rel, 1)
    page = re.sub(r'\s*<nav[^>]*>.*?</nav>', "", page, flags=re.S)
    nav = "\n  " + gbnav(rel, here) + "\n"
    m = re.search(r'<header>.*?</header>', page, flags=re.S)
    if m and "<h1" in m.group(0):
        return page[:m.end()] + nav + page[m.end():]
    m = re.search(r'</h1>', page)
    return page[:m.end()] + nav + page[m.end():]
