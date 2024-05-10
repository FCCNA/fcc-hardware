import ROOT
simul = ROOT.TFile.Open("simul.root", "RECREATE")
import Classe
import funz

'''
#Parametri custom sistema, lunghezze in m, U/R = faccia superiore (cristallo verticale) o destra (cristallo orizzontale), D/L = faccia inferiore o sinistra
nome = 'test'
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

sys = System(nome, dim_x_trigger_superiore, dim_y_trigger_superiore, dim_x_trigger_inferiore,
             dim_y_trigger_inferiore, distanza_tra_trigger, distanza_trigger_superiore_scintillatore,
             rapporto_area_sipm/faccia_U/R, rapporto_area_sipm/faccia_D/L, indice_rifrazione_ambiente, verticale)

'''


#Scegli tempo, apparato e scintillatore
time = 72
sistema = Classe.sysV
scintillatore = Classe.BGO

#Run
muon_df=funz.run(sistema, scintillatore, time, filtro1 = 900, filtro2 = 900)



carica0u = muon_df[0]
carica0d=muon_df[1]
caricau = muon_df[2]
caricad=muon_df[3]


simul.WriteObject(carica0d, "carica0")
simul.WriteObject(caricad, "carica")

simul.WriteObject(carica0u, "vcarica0")
simul.WriteObject(caricau, "vcarica")
