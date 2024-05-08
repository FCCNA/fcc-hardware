import numpy as np
import pandas as pd
import ROOT
from scipy.interpolate import interp1d

import Plot
import Classe

def run(sis, scin, time, Draw_3D = False, Draw_per_faccia = False):
    
    #Inizializzazione, trova numero di muoni ed i loro angoli di incidenza
    expected_muons_min = sis.tup_x * sis.tup_y * 9500
    n = int(expected_muons_min * time * 60)
    th = np.arccos((1 - np.random.random(n)) ** (1/3))
    ph = 2*np.pi * np.random.random(n)
    st = np.sin(th)
    ct = np.cos(th)
    sp = np.sin(ph)
    cp = np.cos(ph)
    
    us, vs, ws = st * cp, st * sp, ct
    
    #Crea raggi ed intersezioni, in questo sistema di riferimento i muoni partono dal basso
    x0s = np.random.rand(n) * sis.tup_x - sis.tup_x / 2
    y0s = np.random.rand(n) * sis.tup_y - sis.tup_y / 2
    z0s = np.zeros(n)
    
    x1s = x0s + sis.dt *st*cp/ct
    y1s = y0s + sis.dt *st*sp/ct
    z1s = np.full(n, sis.dt)
    
    xd = x0s + sis.tup_scint *st*cp/ct
    yd = y0s + sis.tup_scint *st*sp/ct
    zd = np.full(n, sis.tup_scint)
    
    xu = x0s + (sis.tup_scint + scin.z) *st*cp/ct
    yu = y0s + (sis.tup_scint + scin.z) *st*sp/ct
    zu = np.full(n,(sis.tup_scint + scin.z))
    
    x1v = np.full(n, scin.x/2)
    y1v = y0s + (scin.x/2 - x0s) *sp/cp
    z1v = z0s + (scin.x/2 - x0s) *ct/(cp*st)
    
    x2v = np.full(n, -scin.x/2)
    y2v = y0s + (-scin.x/2 - x0s) *sp/cp
    z2v = z0s + (-scin.x/2 - x0s) *ct/(cp*st)
    
    x1l = x0s + (-scin.y/2 - y0s) *cp/sp
    y1l = np.full(n,-scin.y/2 )
    z1l = z0s + (-scin.y/2 - y0s) *ct/(sp*st)
    
    x2l = x0s + (scin.y/2 - y0s) *cp/sp
    y2l = np.full(n,scin.y/2 )
    z2l = z0s + (scin.y/2 - y0s) *ct/(sp*st)
    
    #Calcola distanze percorse
    d1 = np.sqrt( (xd - xu )**2 + (yd - yu ) **2 + ( zd - zu )**2 )
    d2 = np.sqrt( (x1v - xu )**2 + (y1v - yu )**2 + ( z1v - zu )**2 )
    d3 = np.sqrt( (x2v - xu )**2 + (y2v - yu )**2 + ( z2v - zu )**2 )
    d4 = np.sqrt( (x1l - xu )**2 + (y1l - yu )**2 + (z1l - zu)**2 )
    d5 = np.sqrt( (x2l - xu )**2 + (y2l - yu )**2 + (z2l - zu)**2 )
    d6 = np.sqrt( (xd - x1v )**2 + (yd - y1v )**2 + ( zd - z1v )**2 )
    d7 = np.sqrt( (xd - x2v )**2 + (yd - y2v )**2 + ( zd - z2v )**2 )
    d8 = np.sqrt( (xd - x1l )**2 + (yd - y1l )**2 + ( zd - z1l )**2 )
    d9 = np.sqrt( (xd - x2l )**2 + (yd - y2l )**2 + ( zd - z2l )**2 )
    d10 = np.sqrt( (x1v - x2v)**2 + (y1v - y2v)**2 + (z1v - z2v)**2 )
    d11 = np.sqrt( (x1v - x1l)**2 + (y1v - y1l)**2 + (z1v - z1l)**2 )
    d12 = np.sqrt( (x1v - x2l)**2 + (y1v - y2l)**2 + (z1v - z2l)**2 )
    d13 = np.sqrt( (x2v - x1l)**2 + (y2v - y1l)**2 + (z2v - z1l)**2 )
    d14 = np.sqrt( (x2v - x2l)**2 + (y2v - y2l)**2 + (z2v - z2l)**2 )
    d15 = np.sqrt( (x1l - x2l)**2 + (y1l - y2l)**2 + (z1l - z2l)**2 )
    
    #Check sul tipo di percorso
    hitmax_trigger = (-sis.tdown_x/2 < x1s) & (x1s < sis.tdown_x/2) & (-sis.tdown_y/2 < y1s) & (y1s < sis.tdown_y/2)
    hitmax_measure_down = (-scin.x/2 < xd) & (xd < scin.x/2) & (-scin.y/2 < yd) & (yd < scin.y/2)
    hitmax_measure_up = (-scin.x/2 < xu) & (xu < scin.x/2) & (-scin.y/2 < yu) & (yu < scin.y/2)
    hitmax_measure_vert1 = ((-scin.y/2 < y1v) & (y1v < scin.y/2) &
                            (sis.tup_scint < z1v) & (z1v < sis.tup_scint + scin.z))
    hitmax_measure_vert2 =  ((-scin.y/2 < y2v) & (y2v < scin.y/2) &
                             (sis.tup_scint < z2v) & (z2v < sis.tup_scint + scin.z))
    hitmax_measure_lat1 = ((-scin.x/2 < x1l) & (x1l < scin.x/2) &
                           (sis.tup_scint < z1l) & (z1l < sis.tup_scint + scin.z))
    hitmax_measure_lat2 = ((-scin.x/2 < x2l) & (x2l < scin.x/2) &
                           (sis.tup_scint < z2l) & (z2l < sis.tup_scint + scin.z))
    
    hitmax_measure1 = hitmax_trigger & hitmax_measure_up & hitmax_measure_down    
    hitmax_measure2 = hitmax_trigger & hitmax_measure_up & hitmax_measure_vert1
    hitmax_measure3 = hitmax_trigger & hitmax_measure_up & hitmax_measure_vert2 
    hitmax_measure4 = hitmax_trigger & hitmax_measure_up & hitmax_measure_lat1
    hitmax_measure5 = hitmax_trigger & hitmax_measure_up & hitmax_measure_lat2
    hitmax_measure6 = hitmax_trigger & hitmax_measure_down & hitmax_measure_vert1
    hitmax_measure7 = hitmax_trigger & hitmax_measure_down & hitmax_measure_vert2
    hitmax_measure8 = hitmax_trigger & hitmax_measure_down & hitmax_measure_lat1
    hitmax_measure9 = hitmax_trigger & hitmax_measure_down & hitmax_measure_lat2
    hitmax_measure10 =  hitmax_trigger & hitmax_measure_vert1 & hitmax_measure_vert2
    hitmax_measure11 =  hitmax_trigger & hitmax_measure_vert1 & hitmax_measure_lat1
    hitmax_measure12 =  hitmax_trigger & hitmax_measure_vert1 & hitmax_measure_lat2
    hitmax_measure13 =  hitmax_trigger & hitmax_measure_vert2 & hitmax_measure_lat1
    hitmax_measure14 =  hitmax_trigger & hitmax_measure_vert2 & hitmax_measure_lat2
    hitmax_measure15 =  hitmax_trigger & hitmax_measure_lat1 & hitmax_measure_lat2

    hitmax_list = [hitmax_measure1, hitmax_measure2, hitmax_measure3, hitmax_measure4, hitmax_measure5,
                   hitmax_measure6, hitmax_measure7, hitmax_measure8, hitmax_measure9, hitmax_measure10,
                   hitmax_measure11, hitmax_measure12, hitmax_measure13, hitmax_measure14, hitmax_measure15]
    
    hitmax_measure = (hitmax_measure1 | hitmax_measure2 | hitmax_measure3 | hitmax_measure4 | hitmax_measure5
                      | hitmax_measure6 | hitmax_measure7 | hitmax_measure8 | hitmax_measure9 | hitmax_measure10
                      | hitmax_measure11 | hitmax_measure12 | hitmax_measure13 | hitmax_measure14 | hitmax_measure15)
    
    numlist = []
    for hitmax in hitmax_list:
        numlist.append(np.count_nonzero(hitmax))
    num_trigger = np.count_nonzero(hitmax_trigger)
    num_measure = np.count_nonzero(hitmax_measure)
    distance_list = [d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15]
    name = ['UP-DOWN', 'DOWN-L1', 'DOWN-L2', 'DOWN-L3', 'DOWN-L4', 'UP-L1', 'UP-L2', 'UP-L3', 'UP-L4',
            'L1-L2', 'L1-L3: ', 'L1-L4: ', 'L2-L3', 'L2-L4', 'L3-L4']

    #Uno sguardo ai primi dati
    print('Numero totale dei raggi', n,
        '\nNumero di intersezioni con rivelatori di trigger: ', num_trigger, 
        '\nNumero di intersezioni con rivelatori di misura: ', num_measure,
        '\nIntersezioni trigger/raggi tot', num_trigger/n *100,'%',
        '\nMeasure/raggi tot', num_measure/n *100,'%',
        '\nAccettanza geometrica', num_measure/num_trigger * 100,'%')
    for i in range(len(numlist)):
        print(f'{name[i]} : ',(numlist[i]/num_measure)*100,'%')
    
    #Calcola numero fotoni e salva in un dataframe
    distance=np.zeros(n)
    for i in range(len(distance_list)):
        distance+=hitmax_list[i]*distance_list[i]


    #EFFICIENZE
    
    #distribuzione di energia in lunghezza nel caso di sciami
    '''
    distances = np.arange(0,max(distance)*1.2,0.001)
    eff_list=[]
    for dis in distances:
        eff_list.append(ROOT.Math.landau_cdf(dis, 0.02, scin.radlen))
    len_eff = interp1d(distances, eff_list, bounds_error=False)
    distanceweff=[]
    for dist in distance:
        x=np.arange(0, dist, 0.001)
        distanceweff.append(np.trapz(len_eff(x),x))
    distance = np.array(distanceweff)
    '''
    
    #Attenuazione
    decad = np.e**((-(scin.z/2))/(scin.radlen*5))
    distance *=decad

    #Efficienza di riflessione
    '''
    pcrit_cr = 1 - np.sqrt(1-(1.55/scin.rifrazione)**2) 
    pcrit_ce = 1 - np.sqrt(1-(1/scin.rifrazione)**2) 
    #prima riflessione
    *= pcrit_cr

    #seconda riflessione
    + (1-geometric_eff)*(pcrit_cr - pcrit_ce)
    
    refl = np.arcsin(1.55/scin.rifrazione)
    distance *=(1 - 2*refl/np.pi)
    '''

    #Dark counting & Saturazione
    '''
    distance *= 1.03
    '''


    #CALCOLO FOTONI
    scint_A = Classe.effSA * sis.geometric_eff * distance * scin.density * scin.dedx * scin.light_yield
    scint_B = Classe.effSB * sis.geometric_eff * distance * scin.density * scin.dedx * scin.light_yield

    chere_A = Classe.effCA * sis.geometric_eff * distance * (1- 1/(scin.rifrazione*0.99)**2)*2*np.pi/137
    chere_B = Classe.effCB * sis.geometric_eff * distance * (1- 1/(scin.rifrazione*0.99)**2)*2*np.pi/137
    chere_A*=0
    chere_B*=2
    
    muon_data = []
    for i in range(n):
        muon_entry = {'trigger': hitmax_trigger[i], 'measure': hitmax_measure[i],
                      'scintA': scint_A[i], 'chereA': chere_A[i],
                      'scintB': scint_B[i], 'chereB': chere_B[i],
                      'photoA': scint_A[i] + chere_A[i], 'photoB': scint_B[i] + chere_B[i]}
        muon_data.append(muon_entry)
    muon_df = pd.DataFrame(muon_data)
    
    #Simulazione dati reali, conversione in carica e test d'ipotesi
    photo_listA = (muon_df.query('trigger'))['photoA'].tolist()
    photo_listB = (muon_df.query('trigger'))['photoB'].tolist()

    scint_listA = (muon_df.query('measure'))['scintA'].tolist()
    scint_listB = (muon_df.query('measure'))['scintB'].tolist()
    chere_listA = (muon_df.query('measure'))['chereA'].tolist()
    chere_listB = (muon_df.query('measure'))['chereB'].tolist()

    hscintA = ROOT.TH1F("hscintA", f"Fotoni di scintillazione {stateA}", 1000, 00, 00)
    hscintB = ROOT.TH1F("hscintB", f"Fotoni di scintillazione {stateB}", 1000, 00, 00)
    hchereA = ROOT.TH1F("hchereA", f"Fotoni Cherenkov {stateA}", 1000, 00, 00)
    hchereB = ROOT.TH1F("hscintB", f"Fotoni Cherenkov {stateB}", 1000, 00, 00)
    
    hist_caricaA0 = ROOT.TH1F("hist_caricaA0", f"Carica totale {stateA}", 100, -600, 30)
    hist_caricaA = ROOT.TH1F("hist_caricaA", f"Carica cut {stateA}", 100, -6000, -10)
    hist_caricaB0 = ROOT.TH1F("hist_caricaB0", f"Carica totale {stateB}", 100, 0, 20)
    hist_caricadB = ROOT.TH1F("hist_caricaB", f"Carica cut {stateB}", 100, 00, 00)
    
    sigma = 0.1
    noise_A = -15
    noise_B = 2.5
    guadagno_A = 0.308
    guadagno_B = 0.5
    
    for i in range(len(photo_listB)):
        if photo_listB[i]<noise_B:
            electrons_B = noise_B + ROOT.gRandom.Landau(guadagno_B*ROOT.gRandom.Poisson(photo_listB[i]), sigma*((photo_listB[i]+noise_B)**0.8))
        else:
            electrons_B = noise_B + ROOT.gRandom.Landau(guadagno_B*ROOT.gRandom.Poisson(photo_listB[i]), sigma*((photo_listB[i])**0.8))

        if photo_listA[i]<noise_A:
            electrons_A = - (noise_A + ROOT.gRandom.Landau(guadagno_A*ROOT.gRandom.Poisson(photo_listA[i]), sigma*((photo_listA[i]+noise_A)**0.8)))
        else:
            electrons_A = - (noise_A + ROOT.gRandom.Landau(guadagno_A*ROOT.gRandom.Poisson(photo_listA[i]), sigma*(photo_listA[i]**0.8)))
        
        hist_carica0u.Fill(electrons_up)
        hist_carica0d.Fill(electrons_down)
        hist_caricau.Fill(electrons_up)
        hist_caricad.Fill(electrons_down)


    for i in range(len(scint_listu)):
        hscintA.Fill(((ROOT.gRandom.Landau(guadagno_A*ROOT.gRandom.Poisson(scint_listA[i]), sigma*(scint_listA[i]**0.8)))/guadagno_A))
        hscintB.Fill(((ROOT.gRandom.Landau(guadagno_B*ROOT.gRandom.Poisson(scint_listB[i]), sigma*(scint_listB[i]**0.8)))/guadagno_B))
        hchereA.Fill(((ROOT.gRandom.Landau(guadagno_A*ROOT.gRandom.Poisson(chere_listA[i]), sigma*(chere_listA[i]**0.8)))/guadagno_A))
        hchereB.Fill(((ROOT.gRandom.Landau(guadagno_B*ROOT.gRandom.Poisson(chere_listB[i]), sigma*(chere_listD[i]**0.8)))/guadagno_B))

    #Test d'ipotesi
    mean = hscintA.GetMean()
    chere_medio = hchereA.GetMean()
    area_tot = hscintA.Integral(0, hscintA.GetNbinsX() + 1)
    alpha = 0.05
    x_cut = 0
    nmax = hscintA.GetBinLowEdge(hscintA.FindLastBinAbove())
    while x_cut < nmax:
        area_from_x_cut = (hscintA.Integral(hscintA.FindBin(x_cut), hscintA.GetNbinsX() + 1))
        if area_from_x_cut / area_tot <= alpha:
            break
        x_cut += 1

    print('vecchio preamp','\nIl numero di fotoni corrispondente è: ', x_cut,'\nmedia ',mean)
    print('# fotoni dai quali siamo sicuri di distinguere i Fotoni cherenkov ', x_cut - mean )
    print('chere med: ',chere_medio)
    x_obs = mean + chere_medio
    area = (hscintA.Integral(hscintA.FindBin(x_obs), hscintA.GetNbinsX() + 1))
    p_value = area / area_tot
    print('p - value: ',p_value)

    mean = hscintB.GetMean()
    chere_medio = hchereB.GetMean()
    area_tot = hscintB.Integral(0, hscintB.GetNbinsX() + 1)
    alpha = 0.05
    x_cut = 0
    nmax = hscintB.GetBinLowEdge(hscintB.FindLastBinAbove())
    while x_cut < nmax:
        area_from_x_cut = (hscintB.Integral(hscintB.FindBin(x_cut), hscintB.GetNbinsX() + 1))
        if area_from_x_cut / area_tot <= alpha:
            break
        x_cut += 1
    
    print('filtro nuovo preamp','\nIl numero di fotoni corrispondente è: ', x_cut,'\nmedia ',mean)
    print('# fotoni dai quali siamo sicuri di distinguere i Fotoni cherenkov ', x_cut - mean )
    print('chere med: ',chere_medio)
    x_obs = mean + chere_medio
    area = (hscintB.Integral(hscintB.FindBin(x_obs), hscintB.GetNbinsX() + 1))
    p_value = area / area_tot
    print('p - value: ',p_value)

    # Opzioni di disegno
    if Draw_3D:
        go=Plot.Draw3D(sis, scin, hitmax_measure, x0s, y0s, z0s, us, vs, ws, x1s, y1s, z1s)
    
    if Draw_per_faccia:
        # In questo è eseguito lo stesso calcolo ma sono separati per tipo di percorso
        li=[]
        for i in range(len(hitmax_list)):
            li.append(hitmax_list[i]*distance_list[i])
        data_per_faccia=[]
        for i in li:
            new=[]
            for h in i:
                if h!=0:
                    photo = h*sis.geometric_eff*(eff_S1 * scin.density * scin.dedx * scin.light_yield + 
                                                 eff_C1 * (1- 1/(scin.rifrazione*0.99)**2)*2*np.pi/137)
                    new.append(photo)
            data_per_faccia.append(new)
        go=Plot.Drawfacce(data_per_faccia)

    a=Plot.Draw(muon_df)

    disegna = [hist_caricaA0, hist_caricaB0, hist_caricaA, hist_caricaB]
    for graph in disegna:
        a = Plot.drawroot(graph)

    
    return (disegna)
