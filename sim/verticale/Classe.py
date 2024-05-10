class System:
    def __init__(self, name, tup_x, tup_y, tdown_x, tdown_y, dist_t, tup_scint, g_effA, g_effB, rifrazione, verticale):
        self.name = name
        self.tup_x = tup_x
        self.tup_y = tup_y
        self.tdown_x = tdown_x
        self.tdown_y = tdown_y
        self.dt = dist_t
        self.tup_scint = tup_scint
        self.g_effA = g_effA
        self.g_effB = g_effB
        self.rifrazione = rifrazione
        self.v = verticale

class Scintillatore:
    def __init__(self, name, dim_z, dim_y, dim_x, density, light_yield, rifrazione, radlen, ene_loss, emissione, trasmittanza):
        self.name = name
        self.z = dim_z
        self.y = dim_y
        self.x = dim_x
        self.density = density
        self.ly = light_yield
        self.rifrazione = rifrazione
        self.radlen = radlen
        self.dedx = ene_loss
        self.e = emissione
        self.t = trasmittanza


# Dati presi in esperimento per avere funzione di efficienza in lambda
import pandas as pd
from scipy.interpolate import interp1d
from numpy import arange, trapz

lmin, lmax = 200, 900 #lambda min e max accettate dal sistema

# Presa dati
sipmA = pd.read_csv('S14460_pde.csv', names=['lambd', 'pde'], sep='; ', decimal=',')
sipmB = pd.read_csv('S14460_pde.csv', names=['lambd', 'pde'], sep='; ', decimal=',')

e_GBO = pd.read_csv(r'emissione/BGO.csv', skiprows=20, skipfooter=50, sep=';', decimal=',', names=['lambd', 'e'])
t_GBO = pd.read_csv('trasmittanza/BGO.asc', names=['lambd', 't'], skiprows=90, sep='\t', decimal='.')

if filtro_sim[0] == True:
    f_palto_A, f_pbasso_A, f_palto_B, f_pbasso_B = filtro_sim[1]

    x = np.arange(f_palto_A, f_pbasso_A, 1)
    data = [ [x[i], 1] for i in range(len(x) ]
    filtroA = pd.DataFrame(data , columns = ['lambd','pde'])

    x = np.arange(f_palto_B, f_pbasso_B, 1)
    data = [ [x[i], 1] for i in range(len(x) ]
    filtroB =pd.DataFrame(data , columns = ['lambd','pde'])
    
else:
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


#Calcolo efficienze
x = np.arange(lmin, lmax, 1)

tot = f_sipmA(x) * t_f(x) * e_f(x) * f_filtroA(x)
eff_SA = trapz(tot,x)

tot = f_sipmB(x) * t_f(x) * e_f(x) * f_filtroB(x)
eff_SB = trapz(tot,x)

tot = f_sipmB(x) * t_f(x) * f_filtroA(x) * (1/x**2)*(10**9)
eff_CA = trapz(tot,x)

tot = f_sipmB(x) * t_f(x) * f_filtroB(x) * (1/x**2)*(10**9)
eff_CB = trapz(tot,x)


#Sistemi
sysV=System('demoverticale', 0.049, 0.049, 0.049, 0.049, 0.12, 0.025, 0.0625, 0.0625, 1.0003, True)
sysO=System('demoorizzontale', 0.049, 0.049, 0.049, 0.049, 0.12, 0.085, 0.0625, 0.0625, 1.0003, False)


#Scintillatori, dim x, y e z sono le dimensioni del cristallo in m, densità in Kg/m^3, light yield in #/MeV, radlen in m, ene_loss in MeV / (kg/ m^2)
dim_z = 0.05
dim_y = 0.012
dim_x = 0.012

BGO=Scintillatore('BGO', dim_z, dim_y, dim_x, 7130, 8200, 2.15, 0.011, 0.1272, 'emissione/BGO.csv', 'trasmittanza/BGO.asc')
PWO=Scintillatore('PWO', dim_z, dim_y, dim_x, 8280, 190, 2.16, 0.009, 0.1225, 'emissione/PWO.csv', 'trasmittanza/PWO.asc')
