all: gtkgrays2.html gtkgrays2.pdf

gtkgrays2.html: gtkgrays2.page about.page
	-gnome-doc-tool html gtkgrays2.page about.page
	-rm index.cache

gtkgrays2.pdf: gtkgrays2.html about.html
	wkhtmltopdf --no-background  gtkgrays2.html about.html  gtkgrays2.pdf

