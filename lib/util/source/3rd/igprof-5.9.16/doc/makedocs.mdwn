# About the documentation

The IgProf documentation is maintained as a collection of [markdown][] pages
in the `doc` directory, which are then compiled to html using
[jekyll](http://jekyllrb.com/).

Each file that has to be processed by jekyll must have the following header:

    ---
    title: some title
    layout: default
    ---

(including the `---`). If it does not, it is copied to the destination
directory as is.

As required by jekyll, the actual template used for the layout can be found in
the `doc/_layout` directory. In particular, at the moment there is only one
layout `doc/_layout/default.html` which is used for all the pages.

See [here][http://wiki.github.com/mojombo/jekyll/template-data] to have
information on how the template works.

Extra configuarion option, as defined by jekyll
[here](http://wiki.github.com/mojombo/jekyll/configuration), can be specified
in the _config.yml file.

In order to compile the documentation one needs to have the jekyll gem
installed via:

    sudo gem install jekyll

then simply go to the doc directory and type:

    jekyll --server

for a self hosted server serving the pages on `localhost:4000`, or simply do

    jekyll

and find a compiled copy of your web page in _site.

[markdown]: http://daringfireball.net/projects/markdown/syntax
[jekyll]: http://jekyllrb.com/
