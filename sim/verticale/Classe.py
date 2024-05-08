class System:
    def __init__(self, name, tup_x, tup_y, tdown_x, tdown_y, distanza_t, tup_scint,
                 geometric_eff, lambda_min, lambda_max):
        self.name = name
        self.tup_x = tup_x
        self.tup_y = tup_y
        self.tdown_x = tdown_x
        self.tdown_y = tdown_y
        self.dt = distanza_t
        self.tup_scint = tup_scint
        self.geometric_eff = geometric_eff
        self.lambda_min = lambda_min
        self.lambda_max = lambda_max

class Scintillatore:
    def __init__(self, name, dim_z, dim_y, dim_x, density, light_yield, rifrazione, radlen, ene_loss):
        self.name = name
        self.z = dim_z
        self.y = dim_y
        self.x = dim_x
        self.density = density
        self.light_yield = light_yield
        self.rifrazione = rifrazione
        self.radlen = radlen
        self.dedx = ene_loss

#Parametri del sistema, distanze in m
'''
tup e tdown sono le dimesioni lungo gli assi x ed y dei trigger in alto ed in basso,
distanza_t è la distanza tra i due trigger,
tup_scint è la distanza tra il trigger superiore e lo scintillatore,
geometric eff è il rapporto tra l'area del rivelatore e la faccia libera del cristallo
'''

tup_x = 0.049
tup_y = 0.049
tdown_x = 0.049
tdown_y = 0.049
distanza_t = 0.12
tup_scint = 0.025
geometric_eff = 0.0625*0.5

sistema1=System('sys', tup_x, tup_y, tdown_x, tdown_y, distanza_t, tup_scint,
                geometric_eff, lambda_min, lambda_max)

# Cristalli, distanze in m, densità in Kg/m^3, light yield in #/MeV
'''
dim x,y e z sono le dimensioni del cristallo,
radlen è la lunghezza di radiazione del materiale.
ene_loss è la perdita di energia del muone al mip nel materiale in MeV / (kg/ m^2)
'''

dim_z = 0.05
dim_y = 0.012
dim_x = 0.012

BGO=Scintillatore('BGO', dim_z, dim_y, dim_x, density=7130, light_yield=8200, rifrazione = 2.15, radlen = 0.011, ene_loss=0.1272)
PWO=Scintillatore('PWO', dim_z, dim_y, dim_x, density=8280, light_yield=190, rifrazione = 2.16, radlen = 0.009, ene_loss=0.1225)




# Dati presi in esperimento per avere funzione di efficienza in lambda
import pandas as pd
from scipy.interpolate import interp1d
from numpy import arange, trapz
import ROOT

lmin, lmax = 200, 900 #lambda min e max accettate dal sistema

# Presa dati
sipmA = pd.read_csv('S14460_pde.csv', names=['lambd', 'pde'], sep='; ', decimal=',')
sipmB = pd.read_csv('S14460_pde.csv', names=['lambd', 'pde'], sep='; ', decimal=',')

e_GBO = pd.read_csv(r'emissione/BGO.csv', skiprows=20, skipfooter=50, sep=';', decimal=',', names=['lambd', 'e'])
t_GBO = pd.read_csv('trasmittanza/BGO.asc', names=['lambd', 't'], skiprows=90, sep='\t', decimal='.')

if filtri_sim
filtroA = pd.read_csv('filtri/BGOug1.asc', names=['lambd', 'pde'], skiprows=90, sep='\t', decimal='.')
filtroB = pd.read_csv('filtri/BGOug1.asc', names=['lambd', 'pde'], skiprows=90, sep='\t', decimal='.')

# Creo funzioni "continue" per le efficienze
f_sipmA = interp1d(sipmA.lambd, sipmA.pde/100, bounds_error=False, fill_value=0)
f_sipmB = interp1d(sipmB.lambd, sipmB.pde/100, bounds_error=False, fill_value=0)

f_filtro = interp1d(filtro.lambd, filtro.pde/100, bounds_error=False, fill_value=0)

t_f = interp1d(t_GBO.lambd, t_GBO.t/100, bounds_error=False, fill_value=0)
e_f = interp1d(e_GBO.lambd, e_GBO.e, bounds_error=False, fill_value=0)

#normalizzo spettro di emissione
x_norm = arange(lmin, lmax, 1)
y_norm = e_f(x_norm)
int_norm = trapz(y_norm,x_norm)
e_f = interp1d(e_GBO.lambd, e_GBO.e/int_norm, bounds_error=False, fill_value=0)


def eff_SA(x):
    tot = f_sipmA(x) * t_f(x) * e_f(x) * f_filtroA(x)
    integrale = trapz(tot,x)
    return(integrale)
    
def eff_SB(x):
    tot = f_sipmB(x) * t_f(x) * e_f(x) * f_filtroB(x)
    integrale = trapz(tot,x)
    return(integrale)

def eff_CA(x):
    tot = f_sipmB(x) * t_f(x) * f_filtroA(x) * (1/x**2)*(10**9)
    integrale = trapz(tot,x)
    return(integrale)

def eff_CB(x):
    tot = f_sipmB(x) * t_f(x) * f_filtroB(x) * (1/x**2)*(10**9)
    integrale = trapz(tot,x)
    return(integrale)


#funzione di eff per distribuzione di enrgia in lunghezza
distances = arange(0,0.06,0.001)
eff_list=[]
for dis in distances:
    eff_list.append(ROOT.Math.landau_cdf(dis, 0.005, 0.004))

len_eff = interp1d(distances, eff_list, bounds_error=False)
