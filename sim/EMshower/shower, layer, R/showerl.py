import sys
from ROOT import TCanvas, TPad, TPaveLabel, TGraph
from array import array
import showerf as sh

def ini():
	startenergy=10000
	criticalenergy=10
	return(startenergy,criticalenergy)

def main(args):
	
	#Initialize
	starte=sh.starte
	pho=sh.par(starte[0],"p",0,0,0,0,0,0)
	
	#Run
	go=sh.launch(pho)
	li=list(go)
	T=len(li)
	
	#Build the graphs
	df=sh.make(li,T)
	hnp = sh.histo1(df["N pho"], T, 'Photon number')
	hne = sh.histo1(df["N ele"], T, 'Elec/posi-tron number')
	he = sh.histo1(df["E"], T, 'Total energy')
	hspace = sh.histo3(li, 'Shower', T, -1, 4*sh.t, -10,10,-10,10)

	lief=sh.enedist(30)
	setx,sety=array('d'), array('d')
	for x in range(len(lief)):
		setx.append(lief[x][0])
		sety.append(lief[x][1])
	hcili = TGraph(len(setx), setx, sety)

	#Draw
	can = TCanvas( 'can', ' ', 200, 10, 1000, 1000 )
	pad1 = TPad( 'pad1', 'histogram1', 0.02, 0.02, 0.50, 0.92, 21 )
	pad2 = TPad( 'pad2', 'histogram2', 0.50, 0.02, 0.98, 0.92, 21 )
	pad1.Draw()
	pad2.Draw()
	title = TPaveLabel( 0.1, 0.94, 0.9, 0.98, 'Particles count' )
	title.SetFillColor( 16 )
	title.SetTextFont( 52 )
	title.Draw()
	draw=sh.drawhisto(pad1,hnp,'X (X0)','Particles')
	draw=sh.drawhisto(pad2,hne,'X (X0)','Particles')
	can.Update()

	can2 = TCanvas( 'can2', ' ', 200, 10, 1000, 1000 )
	pad = TPad( 'pad', 'histogram3', 0.02, 0.50, 0.98, 0.98, 21 )
	pad4 = TPad( 'pad4', 'graph', 0.02, 0.02, 0.98, 0.50, 21 )
	pad.Draw()
	pad4.Draw()
	draw2=sh.drawhisto(pad,he,'X (X0)','Energy')
	draw=sh.drawgraph(pad4,hcili)
	can2.Update()

	can3 = TCanvas( 'can3', ' ', 200, 10, 1000, 1000 )
	pad3 = TPad( 'pad3', 'histogram4', 0.02, 0.02, 0.98, 0.98, 21 )
	pad3.Draw()
	draw=sh.drawhisto3(pad3,hspace)
	can3.Update()

	#info
	print('rad',T)
	run=sh.sea(li,True)
	return(0)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
