import math
import ROOT
import random
import pandas as pd
from array import array
from copy import deepcopy as dc
from showernolat import ini

lie=[]
starte=ini()
t=math.log(starte[0]/starte[1],(2))
R=2.1
ligy=[]
ligz=[]

class par():
	def __init__(self,e,t,d,x,y,z,rx,ry,rz):
		self.e=e
		self.t=t
		self.d=d
		self.x=x
		self.y=y
		self.z=z
		self.rx=rx
		self.ry=ry
		self.rz=rz
		
	def __str__(self):
		return f"{self.e}({self.t})"

def pair(x):
	a,b= dc(x),dc(x)
	a.e/= 2
	b.e/= 2
	a.t, b.t= "e", "e"
	a.d, b.d= 0, 0
	return([a,b])

def brem(x):
	a,b= dc(x),dc(x)
	step=R/t
	s= random.gauss(0,step)
	angle=random.random()*2*math.pi
	ys= s*math.sin(angle)
	zs= s*math.cos(angle)
	xs= math.sqrt(1 - (ys*ys + zs*zs))
	ligy.append(s)
	ligz.append(zs)
	
	a.e*=(1 - 1/math.e)
	b.e/=math.e
	a.t="p"
	a.d=0
	b.d=0
	b.rx=xs
	b.ry=ys
	b.rz=zs
	a.rx=xs
	a.ry=ys
	a.rz=zs
	return([a,b])

def move(x):
	x.x+=x.rx/7
	x.y+=x.ry/7
	x.z+=x.rz/7
	x.d+=1
	return(x)

def adv(p,e):
	p2=[]
	e2=[]
	#crea copia ed avanza il tempo
	for h in range(len(p)):
		p2.append(dc(p[h]))
		p2[h]=move(p2[h])
	for h in range(len(e)):
		e2.append(dc(e[h]))
		e2[h]=move(e2[h])
	#fa accadere eventi
	for h in range(len(p2)):
		if  p2[h].d >=9 and p2[h].e > starte[1]:
			e2.extend(pair(p[h]))
			p2[h]='nan'
	for h in range(len(e2)):
		if e2[h].d >= 7 and e2[h].e > starte[1]:
			k=brem(e2[h])
			e2.append(k[1])
			p2.append(k[0])
			e2[h]='nan'
	#pulisco lista
	h=0
	while h < len(p2):
		if p2[h]=='nan':
			p2.pop(h)
		elif p2[h].e < starte[1]*5 and p2[h].d >=random.gauss(7*8,7):
			lie.append(p2[h])
			p2.pop(h)
		else:
			h+=1
	h=0
	while h < len(e2):
		if e2[h]=='nan':
			e2.pop(h)
		elif e2[h].e< starte[1]*5 and e2[h].d >=random.gauss(7,0.5):
			lie.append(e2[h])
			e2.pop(h)
		else:
			h+=1
	return(p2,e2)

def launch(part):
	e=[]
	p=[]
	if part.t=='e':
		e.append(part)
	if part.t=='p':
		p.append(part)
	li=[[p,e],]
	stop=True
	while stop:
		a=adv(p,e)
		a=list(a)
		li.append(a)
		p=a[0]
		e=a[1]
		if len(a[0])==0 and len(a[1])==0:
			stop=False
	return(li)

#Raggruppo i dati per i grafici
def make(li):
	df = pd.DataFrame([],columns=['N pho','N ele','E'])
	h=-1
	for i in range(len(li)):		
		df.at[i,'N pho']=len(li[i][0])
		df.at[i,'N ele']=len(li[i][1])
		E=0
		for k in range(len(li[i][0])):
			E+=li[i][0][k].e
		for k in range(len(li[i][1])):
			E+=li[i][1][k].e
		df.at[i,'E']=E
	return(df)

def enedist(tick):
	raggio=3
	final=[]
	r=0
	while r < raggio:
		r+=raggio/tick
		E=0
		for h in range(len(lie)):
			rpart=math.sqrt(lie[h].y*lie[h].y + lie[h].z*lie[h].z)
			if rpart <= r:
				E+=lie[h].e
		a=[r,E/starte[0]]
		final.append(a)
	return(final)

#Disegno
def histo1(data, T, name):
	histo = ROOT.TH1F('histo',name,T+10, 0, T+10)
	histo.SetDirectory(0)
	for x in range(T):
		for h in range(int(data[x])):
			histo.Fill(x)
	return(histo)

def histo3(data, name, bins, xmin, xmax, ymin, ymax, zmin, zmax):
	histo = ROOT.TH3F('histo',name, bins, xmin, xmax, bins, ymin, ymax, bins, zmin, zmax)
	histo.SetDirectory(0)
	for batch in data:
		for lis in batch:
			for part in lis:
				histo.Fill(part.x,part.y,part.z)
	return(histo)

def histod(data, name, bins, xmin, xmax, ymin, ymax, zmin, zmax):
	histo = ROOT.TH3F('histo',name, bins, xmin, xmax, bins, ymin, ymax, bins, zmin, zmax)
	histo.SetDirectory(0)
	for part in data:
		histo.Fill(part.x,part.y,part.z)
	return(histo)

def graph(tick):
	lief=enedist(tick)
	setx,sety=array('d'), array('d')
	for x in range(len(lief)):
		setx.append(lief[x][0])
		sety.append(lief[x][1])
	graph = ROOT.TGraph(len(lief), setx, sety)
	return(graph)
	
def drawhisto(pad,histo,namex,namey):
	pad.cd()
	histo.SetFillColor( 5 )
	histo.SetStats(0)
	histo.GetXaxis().SetTitle(namex)
	histo.GetYaxis().SetTitle(namey)
	histo.Draw()
	return(0)

def drawgraph(pad,graph,title):
	pad.cd()
	graph.SetTitle(title)
	graph.GetXaxis().SetTitle('Raggio (X0)')
	graph.GetYaxis().SetTitle('Energia (MeV)')
	graph.Draw()
	return(0)

def drawhisto3(pad,histo):
	pad.cd()
	histo.SetFillColor( 5 )
	histo.SetStats(0)
	histo.GetXaxis().SetTitle('x (X0)')
	histo.GetYaxis().SetTitle('y (X0)')
	histo.GetYaxis().SetTitle('z (X0)')
	histo.Draw("")
	return(0)

#funzione ricerca
def sea(li,c):
	if c:
		n=int(input('time ',))
		print('al tempo ',n,' i fotoni sono ')
		for x in range(len(li[n][0])):
			print(li[n][0][x])
		print('mentre gli elettroni e positroni sono ')
		for x in range(len(li[n][1])):
			print(li[n][1][x])
		print('controllare un altro tempo? s/n')
	i=input()
	if i=='s':
		show=sea(li,True)
	elif i=='n':
		return(0)
	else:
		print('input invalido')
		show=sea(li,False)
	return(0)
