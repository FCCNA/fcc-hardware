#Parametri del sistema, lunghezze in m, U/R = faccia superiore (cristallo verticale) o destra (cristallo orizzontale), D/L = faccia inferiore o sinistra

dim_x_trigger_superiore = 0.049
dim_y_trigger_superiore = 0.049
dim_x_trigger_inferiore = 0.049
dim_y_trigger_inferiore = 0.049
distanza_tra_trigger = 0.12
distanza_trigger_superiore_scintillatore = 0.025
rapporto_area_sipm/faccia_U/R = 0.0625
rapporto_area_sipm/faccia_D/L = 0.0625
indice_rifrazione_ambiente = 1.0003 #ex. aria
verticale = True


# Cristallo: dim x, y e z sono le dimensioni del cristallo in m
altezza = 0.05
larghezza = 0.012 # asse y
lunghezza = 0.012 # asse x


#Fine edit
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
    def __init__(self, name, dim_z, dim_y, dim_x, density, light_yield, rifrazione, radlen, ene_loss):
        self.name = name
        self.z = dim_z
        self.y = dim_y
        self.x = dim_x
        self.density = density
        self.ly = light_yield
        self.rifrazione = rifrazione
        self.radlen = radlen
        self.dedx = ene_loss


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
sysO=System('demoverticale', 0.049, 0.049, 0.049, 0.049, 0.12, 0.025, 0.0625, 0.0625, 1.0003, True)

dim_x_trigger_superiore = 0.049
dim_y_trigger_superiore = 0.049
dim_x_trigger_inferiore = 0.049
dim_y_trigger_inferiore = 0.049
distanza_tra_trigger = 0.12
distanza_trigger_superiore_scintillatore = 0.025
rapporto_area_sipm/faccia_U/R = 0.0625
rapporto_area_sipm/faccia_D/L = 0.0625
indice_rifrazione_ambiente = 1.0003 #ex. aria
verticale = True


#Scintillatori, densità in Kg/m^3, light yield in #/MeV, radlen in m, ene_loss in MeV / (kg/ m^2)
BGO=Scintillatore('BGO', dim_z, dim_y, dim_x, density=7130, light_yield=8200, rifrazione = 2.15, radlen = 0.011, ene_loss=0.1272)
PWO=Scintillatore('PWO', dim_z, dim_y, dim_x, density=8280, light_yield=190, rifrazione = 2.16, radlen = 0.009, ene_loss=0.1225)
