# Build the paper: three pdflatex passes (cross-references and the longtable need them).
# Requires a TeX Live installation with lmodern, microtype, booktabs, tabularx, longtable, caption, enumitem, hyperref.
all: main.pdf

main.pdf: main.tex
	pdflatex -interaction=nonstopmode -halt-on-error main.tex
	pdflatex -interaction=nonstopmode -halt-on-error main.tex
	pdflatex -interaction=nonstopmode -halt-on-error main.tex

clean:
	rm -f main.aux main.log main.out main.toc build.log

.PHONY: all clean
