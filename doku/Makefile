DOKU = dls_doku

first: $(DOKU).dvi $(DOKU).ps $(DOKU).pdf

$(DOKU).dvi: dls_doku.tex bilder/*.eps Makefile
	latex --file-line-error-style '\nonstopmode\input{$(DOKU).tex}'
	latex '\batchmode\input{$(DOKU).tex}'
	makeindex $(DOKU).idx
	latex '\batchmode\input{$(DOKU).tex}'

$(DOKU).ps: $(DOKU).dvi
	dvips -t a4 $(DOKU).dvi

$(DOKU).pdf: $(DOKU).ps
	ps2pdf $(DOKU).ps

clean:
	rm -f $(DOKU).aux $(DOKU).dvi $(DOKU).idx \
		$(DOKU).ilg $(DOKU).ind $(DOKU).log \
		$(DOKU).out $(DOKU).pdf $(DOKU).ps \
		$(DOKU).toc