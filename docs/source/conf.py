# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'compi'
copyright = '2025, cmelnu'
author = 'cmelnu'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = []

templates_path = ['_templates']
exclude_patterns = []

import sphinx_rtd_theme


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_theme_options = {
    'navigation_depth': 5,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'includehidden': True,
    'titles_only': False,
    'style_nav_header_background': '#2980B9',  # Blue header
}

# Custom CSS for better code block styling
html_static_path = ['_static']
html_css_files = [
    'custom.css',
]

# Show "Edit on GitHub" link
html_context = {
    'display_github': True,
    'github_user': 'cmelnu',
    'github_repo': 'compi',
    'github_version': 'main',
    'conf_py_path': '/docs/source/',
}

# Syntax highlighting
pygments_style = 'monokai'  # Dark theme for code blocks

