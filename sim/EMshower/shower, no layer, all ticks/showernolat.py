import sys
from ROOT import TCanvas, TPad, TH1F
import showerf as sh

def ini():
	startenergy=2000 #MeV
	criticalenergy=10 #MeV
	return(startenergy,criticalenergy)

def main(args):
	
	#Initialize
	starte=ini()
	pho=sh.par(starte[0],"e",0,0,0,0,1,0,0)
	
	#Run
	li=list(sh.launch(pho))
	T=len(li)
	
	#Creo grafici
	df=sh.make(li)
	hnp = sh.histo1(df["N pho"], T, 'Conteggio fotoni')
	hne = sh.histo1(df["N ele"], T, 'Conteggio elet/posi-troni')
	he = sh.histo1(df["E"], T, 'Energia totale (MeV)')
	space=sh.histo3(li,'Shower',100,0,T/7 +3,-10,10,-10,10)
	spaced=sh.histod(sh.lie,'Showerd',100,0,T/7 +3,-10,10,-10,10)
	hcili = sh.graph(30)
	
	ligy=sh.ligy
	ligz=sh.ligz
	hligy=TH1F('hligy','hligy',100,-1,1)
	hligz=TH1F('hligz','hligz',100,-1,1)
	for x in ligy:
		hligy.Fill(x)
	for x in ligz:
		hligz.Fill(x)
	
	#Disegno
	can = TCanvas( 'can', ' ', 200, 10, 1000, 1000 )
	pad = TPad( 'pad', 'histogram1', 0.02, 0.02, 0.50, 0.98, 21 )
	pad1 = TPad( 'pad1', 'histogram2', 0.50, 0.02, 0.98, 0.98, 21 )
	pad.Draw()
	pad1.Draw()
	draw=sh.drawhisto(pad,hnp,'X (X0/7)','Particlelle')
	draw=sh.drawhisto(pad1,hne,'X (X0/7)','Particlelle')
	can.Update()
	
	can1 = TCanvas( 'can1', ' ', 200, 10, 1000, 1000 )
	pad2 = TPad( 'pad2', 'histogram3', 0.02, 0.50, 0.98, 0.98, 21 )
	pad3 = TPad( 'pad3', 'graph', 0.02, 0.02, 0.98, 0.50, 21 )
	pad2.Draw()
	pad3.Draw()
	draw=sh.drawhisto(pad2,he,'X (X0/7)','Energia (MeV)')
	draw=sh.drawgraph(pad3,hcili,'Raggio del cilindro vs Energia contenuta')
	can1.Update()

	can2 = TCanvas( 'can2', ' ', 200, 10, 1000, 1000 )
	pad4 = TPad( 'pad4', 'histogram4', 0.02, 0.02, 0.98, 0.98, 21 )
	pad4.Draw()
	draw=sh.drawhisto3(pad4,space)
	can2.Update()
	
	can4 = TCanvas( 'can4', ' ', 200, 10, 1000, 1000 )
	pad7 = TPad( 'pad7', 'histogram7', 0.02, 0.02, 0.98, 0.98, 21 )
	pad7.Draw()
	draw=sh.drawhisto3(pad7,spaced)
	can4.Update()
	
	can3 = TCanvas( 'can3', ' ', 200, 10, 1000, 1000 )
	pad5 = TPad( 'pad5', 'histogram5', 0.02, 0.50, 0.98, 0.98, 21 )
	pad6 = TPad( 'pad6', 'histogram6', 0.02, 0.02, 0.98, 0.50, 21 )
	pad5.Draw()
	pad6.Draw()
	draw=sh.drawhisto(pad5,hligy,'y','dist')
	draw=sh.drawhisto(pad6,hligz,'z','dist')
	can3.Update()

	#info
	print('tempi eventi ',T)
	run=sh.sea(li,True)
	return(0)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

