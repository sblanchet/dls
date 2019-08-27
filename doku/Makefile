#------------------------------------------------------------------------------
#
#  LaTeX Makefile
#
#------------------------------------------------------------------------------

LATEX_OPTIONS := -file-line-error-style

#------------------------------------------------------------------------------

first: dvi

all: dvi pdf

dvi: dls_doku.dvi dls_doku_en.dvi

pdf:  dls_doku.pdf dls_doku_en.pdf

%.dvi : %.tex
	latex $(LATEX_OPTIONS) \\nonstopmode\\input{$<}

%.pdf : %.tex
	pdflatex $(LATEX_OPTIONS) $<


clean:
	@rm -f *.aux *.dvi *.idx \
			*.ilg *.ind *.log \
			*.out *.pdf *.ps \
			*.toc *.lot *.lof \
			*.lol *.glo \
			*.bak bilder/*.bak

.PHONY:  first dvi all pdf clean

#------------------------------------------------------------------------------
