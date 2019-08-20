#------------------------------------------------------------------------------
#
#  LaTeX Makefile
#
#------------------------------------------------------------------------------

LATEX_OPTIONS := -file-line-error-style

#------------------------------------------------------------------------------

first: dvi

all: dvi pdf

dvi: dls_doku.dvi dls_doku_en.dvi dls_doku_fr.dvi

pdf:  dls_doku.pdf dls_doku_en.pdf dls_doku_fr.pdf

%.dvi : %.tex
#	./update-hg.sh
	latex $(LATEX_OPTIONS) \\nonstopmode\\input{$<}
	makeindex $(patsubst %.tex,%.idx,$<)
	latex $(LATEX_OPTIONS) \\nonstopmode\\input{$<}

%.pdf : %.tex
#	./update-hg.sh
	latex $(LATEX_OPTIONS) \\nonstopmode\\input{$<}
	makeindex $(patsubst %.tex,%.idx,$<)
	pdflatex $(LATEX_OPTIONS) $<


clean:
	@rm -f *.aux *.dvi *.idx \
			*.ilg *.ind *.idx *.log \
			*.out *.pdf *.ps \
			*.toc *.lot *.lof \
			*.lol *.glo \
			*.bak bilder/*.bak \
			bilder/*-eps-converted-to.pdf

#hg.tex

.PHONY:  first dvi all pdf clean

#------------------------------------------------------------------------------
