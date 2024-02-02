import math
import ROOT
import random
import pandas as pd
from copy import deepcopy as dc
from showerl import ini

lie=[]
starte=ini()
t=math.log(starte[0]/starte[1],(2))
R=2.1

class par():
	def __init__(self,e,t,d,x,y,z,ry,rz):
		self.e=e
		self.t=t
		self.d=d
		self.x=x
		self.y=y
		self.z=z
		self.ry=ry
		self.rz=rz
	
	def __str__(self):
		return f"{self.e}({self.t})"


def pair(x):
	a=dc(x)
	b=dc(x)
	a.e/=2
	b.e/=2
	a.t="e"
	b.t="e"
	a.d=0
	b.d=0
	return ([a,b])

def brem(x):
	a=dc(x)
	b=dc(x)
	step=R/t
	s=abs(random.gauss(0,step/2))
	angle=random.random()*2*math.pi
	sin=math.sin(angle)
	cos=math.cos(angle)
	
	a.e*=(1- 9/(7*math.e))
	b.e*=9/(7*math.e)
	a.t="p"
	a.d=0
	b.d=0
	b.ry=s*sin
	b.rz=s*cos
	return ([a,b])
	
def move(x):
	x.x+=1
	x.y+=x.ry
	x.z+=x.rz
	x.d+=1
	return(x)
	
def layer(x):
	x=move(x)
	if x.e > starte[1]:
		if x.t == "e":
			a=brem(x)
		else:
			a=pair(x)
	else:
		if x.t == "e":
			a=0
			lie.append(x)
		else:
			if x.d >8 and random.random()<0.5:
				a=0
				lie.append(x)
			else:
				y=dc(x)
				a=[y]
	return(a)
	
def adv(batch):
	batch2=[]
	for p in batch:
		new=layer(p)
		if new!=0:
			batch2.extend(new)
	return(batch2)

def launch(part):
	li=[[part],]
	while len(li[-1])>0:
		a=adv(li[-1])
		a=list(a)
		li.append(a)
	return(li)


#Build data for graph
def enedist(tick):
	radius=3
	final=[]
	r=0
	while r < radius:
		r+=radius/tick
		E=0
		for h in range(len(lie)):
			rpart=math.sqrt(lie[h].y*lie[h].y + lie[h].z*lie[h].z)
			if rpart <= r:
				E+=lie[h].e
		a=[r,E/starte[0]]
		final.append(a)
	return(final)

def chose(batch,c):
	batch2=[]
	if c=='p':
		for h in range(len(batch)):
			if batch[h].t=='p':
				batch2.append(batch[h])
	if c=='e':
		for h in range(len(batch)):
			if batch[h].t=='e':
				batch2.append(batch[h])
	return(batch2)

def make(li,T):
	df = pd.DataFrame([],columns=['N pho','N ele','E'])
	for h in range(T):
		df.at[h,'N pho']=len(chose(li[h],'p'))
		df.at[h,'N ele']=len(chose(li[h],'e'))
		E=0
		for k in range(len(li[h])):
			E+=li[h][k].e
		df.at[h,'E']=E
	return(df)
	

#Drawing section
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
		for ele in batch:
			histo.Fill(ele.x,ele.y,ele.z)
	return(histo)

def drawhisto(pad,histo,namex,namey):
	pad.cd()
	histo.SetFillColor( 5 )
	histo.SetStats(0)
	histo.GetXaxis().SetTitle(namex)
	histo.GetYaxis().SetTitle(namey)
	histo.Draw()
	return(0)

def drawgraph(pad,graph):
	pad.cd()
	graph.SetTitle('Cilinder radius vs Energy contained')
	graph.GetXaxis().SetTitle('Radius (X0)')
	graph.GetYaxis().SetTitle('Energy')
	graph.Draw()
	return(0)

def drawhisto3(pad,histo):
	pad.cd()
	histo.SetFillColor( 5 )
	histo.SetStats(0)
	histo.GetXaxis().SetTitle('x (X0)')
	histo.GetYaxis().SetTitle('y (X0)')
	histo.GetYaxis().SetTitle('z (X0)')
	histo.Draw()
	return(0)

#search function
def sea(li,c):
	if c:
		n=int(input('step ',))
		print('at the ',n,' step the particles are ')
		for x in range(len(li[n])):
			print(li[n][x])
		print('check another step? y/n')
	i=input()
	if i=='y':
		show=sea(li,True)
	elif i=='n':
		return(0)
	else:
		print('invalid input')
		show=sea(li,False)
	return(0)
