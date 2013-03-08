OUT = report

all: $(OUT).ps $(OUT).pdf

$(OUT).dvi: *.tex #*.eps
	pdflatex $*
	bibtex $*
	latex $*
	latex $*

# -j0 -G0: embed all fonts
$(OUT).ps: $(OUT).dvi #*.eps
	dvips -Ppdf -t letter -j0 -G0 -o $@ $<

# -d options: embed all fonts (use 'pdffonts' util to verify)
$(OUT).pdf: $(OUT).ps
	ps2pdf -dCompatibilityLevel=1.4 -dPDFSETTINGS=/prepress $<

clean:
	rm -f *~ *.aux *.dvi *.bbl *.blg *.log
