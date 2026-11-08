# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see
# https://www.sphinx-doc.org/en/master/usage/configuration.html

project = "PerfRegions"
copyright = "2026, PerfRegions authors"  # noqa: A001
author = "The PerfRegions authors"
version = "dev"
release = version

# -- General configuration ---------------------------------------------------

extensions = [
    "sphinx_design",
    "sphinx_copybutton",
]

templates_path = []
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# -- Options for HTML output -------------------------------------------------

html_theme = "pydata_sphinx_theme"
html_title = "PerfRegions Documentation"
html_theme_options = {
    "navbar_start": ["navbar-logo"],
    "navbar_center": ["navbar-nav"],
    "navbar_end": ["theme-switcher"],
    "navbar_persistent": ["search-button"],
    "logo": {
        "text": "PerfRegions",
        "alt_text": "PerfRegions Documentation - Home",
    },
    "show_nav_level": 3,
    "navigation_depth": 4,
    "collapse_navigation": True,
    "secondary_sidebar_items": [],
}
