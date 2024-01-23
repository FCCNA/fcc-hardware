import numpy as np
import pandas as pd
import ROOT
from scipy.interpolate import interp1d

import Plot
import Classe

def run(sis, scin, time, filtro1 = 900, filtro2 = 900, Draw_3D = False, Draw_per_faccia = False):
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
    
    hitmax_measure = (hitmax_measure1 | hitmax_measure2 | hitmax_measure3 | hitmax_measure4 | hitmax_measure5 | hitmax_measure6 | hitmax_measure7 | hitmax_measure8 | hitmax_measure9 | hitmax_measure10 | hitmax_measure11 | hitmax_measure12 | hitmax_measure13 | hitmax_measure14 | hitmax_measure15)
    
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
    
    x = np.arange(200, filtro1, 1)
    eff_S1 = Classe.effxlambdaSf(x)
    eff_C1 = Classe.effxlambdaCf(x)
    
    x = np.arange(200, filtro2, 1)
    eff_S2 = Classe.effxlambdaS(x)
    eff_C2 = Classe.effxlambdaC(x)
    
    #funzione di eff per distribuzione di enrgia in lunghezza
    distances = np.arange(0,max(distance)*1.2,0.001)
    eff_list=[]
    for dis in distances:
        eff_list.append(ROOT.Math.landau_cdf(dis, 0.02, scin.radlen))
    len_eff = interp1d(distances, eff_list, bounds_error=False)
    distanceweff=[]
    for dist in distance:
        x=np.arange(0, dist, 0.001)
        distanceweff.append(np.trapz(len_eff(x),x))
    #distance = np.array(distanceweff)
    
    #funzione di assorbimento
    decad = np.e**((-(scin.x/2))/(scin.radlen*5))
    distance *=decad
    
    scint_phor = eff_S1 * sis.geometric_eff * distance * scin.density * scin.dedx * scin.light_yield
    chere_phor = eff_C1 * sis.geometric_eff * distance * (1- 1/(scin.rifrazione*0.99)**2)*2*np.pi/137

    scint_phol = eff_S2 * sis.geometric_eff * distance * scin.density * scin.dedx * scin.light_yield
    chere_phol = eff_C2 * sis.geometric_eff * distance * (1- 1/(scin.rifrazione*0.99)**2)*2*np.pi/137
    
    muon_data = []
    for i in range(n):
        muon_entry = {'trigger': hitmax_trigger[i], 'measure': hitmax_measure[i],
                      'scintr': scint_phor[i], 'cherer': chere_phor[i],
                      'scintl': scint_phol[i], 'cherel': chere_phol[i],
                      'photol': scint_phol[i] + chere_phol[i], 'photor': scint_phor[i] + chere_phor[i]}
        muon_data.append(muon_entry)
    muon_df = pd.DataFrame(muon_data)
    
    #Simulazione dati reali, conversione in carica e test d'ipotesi
    photo_listr = (muon_df.query('trigger'))['photor'].tolist()
    photo_listl = (muon_df.query('trigger'))['photol'].tolist()

    scint_listr = (muon_df.query('measure'))['scintr'].tolist()
    scint_listl = (muon_df.query('measure'))['scintl'].tolist()
    chere_listr = (muon_df.query('measure'))['cherer'].tolist()
    chere_listl = (muon_df.query('measure'))['cherel'].tolist()

    hscintr = ROOT.TH1F("hscintr", "Scintr", 1000, 00, 00)
    hscintl = ROOT.TH1F("hscintl", "Scintl", 1000, 00, 00)
    hcherer = ROOT.TH1F("hcherer", "Cherer", 1000, 00, 00)
    hcherel = ROOT.TH1F("hscintl", "Scintl", 1000, 00, 00)
    
    hist_carica0r = ROOT.TH1F("hist_carica0r", "Carica0r", 100, 00, 00)
    hist_caricar = ROOT.TH1F("hist_caricar", "Caricar", 100, 10, 00)
    
    hist_carica0l = ROOT.TH1F("hist_carica0l", "Carica0l", 100, 00, 00)
    hist_carical = ROOT.TH1F("hist_carical", "Carical", 100, 00, -10)
    
    sigma = 0.1
    noisel = -12
    noiser = 2.5
    guadagnor = 0.5
    guadagnol = 0.308
    
    for ele in photo_listr:
        electrons = noiser + ROOT.gRandom.Landau(guadagnor*ROOT.gRandom.Poisson(ele),sigma*((abs(ele)+noiser)**0.8))
        hist_carica0r.Fill(electrons)
        hist_caricar.Fill(electrons)

    for ele in photo_listl:
        electrons = - (noisel + ROOT.gRandom.Landau(guadagnol*ROOT.gRandom.Poisson(ele),sigma*(ele**0.8)))
        hist_carica0l.Fill(electrons)
        hist_carical.Fill(electrons)


    for i in range(len(scint_listr)):
        hscintr.Fill(((ROOT.gRandom.Landau(guadagnor*ROOT.gRandom.Poisson(scint_listr[i]),
                                           sigma*(scint_listr[i]**0.8)))/guadagnor))
        hscintl.Fill(((ROOT.gRandom.Landau(guadagnol*ROOT.gRandom.Poisson(scint_listl[i]),
                                           sigma*(scint_listl[i]**0.8)))/guadagnol))
        hcherer.Fill(((ROOT.gRandom.Landau(guadagnor*ROOT.gRandom.Poisson(chere_listr[i]),
                                           sigma*(chere_listr[i]**0.8)))/guadagnor))
        hcherel.Fill(((ROOT.gRandom.Landau(guadagnol*ROOT.gRandom.Poisson(chere_listl[i]),
                                           sigma*(chere_listl[i]**0.8)))/guadagnol))

    #Test d'ipotesi
    mean = hscintr.GetMean()
    chere_medio = hcherer.GetMean()
    area_tot = hscintr.Integral(0, hscintr.GetNbinsX() + 1)
    alpha = 0.05
    x_cut = 0
    nmax = hscintr.GetBinLowEdge(hscintr.FindLastBinAbove())
    while x_cut < nmax:
        area_from_x_cut = (hscintr.Integral(hscintr.FindBin(x_cut), hscintr.GetNbinsX() + 1))
        if area_from_x_cut / area_tot <= alpha:
            break
        x_cut += 1

    print('filtro nuovo preamp','\nIl numero di fotoni corrispondente è: ', x_cut,'\nmedia ',mean)
    print('# fotoni dai quali siamo sicuri di distinguere i Fotoni cherenkov ', x_cut - mean )
    print('chere med: ',chere_medio)
    x_obs = mean + chere_medio
    area = (hscintr.Integral(hscintr.FindBin(x_obs), hscintr.GetNbinsX() + 1))
    p_value = area / area_tot
    print('p - value: ',p_value)

    mean = hscintl.GetMean()
    chere_medio = hcherel.GetMean()
    area_tot = hscintl.Integral(0, hscintl.GetNbinsX() + 1)
    alpha = 0.05
    x_cut = 0
    nmax = hscintl.GetBinLowEdge(hscintl.FindLastBinAbove())
    while x_cut < nmax:
        area_from_x_cut = (hscintl.Integral(hscintl.FindBin(x_cut), hscintl.GetNbinsX() + 1))
        if area_from_x_cut / area_tot <= alpha:
            break
        x_cut += 1
    
    print('vecchio preamp','\nIl numero di fotoni corrispondente è: ', x_cut,'\nmedia ',mean)
    print('# fotoni dai quali siamo sicuri di distinguere i Fotoni cherenkov ', x_cut - mean )
    print('chere med: ',chere_medio)
    x_obs = mean + chere_medio
    area = (hscintl.Integral(hscintl.FindBin(x_obs), hscintl.GetNbinsX() + 1))
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
                    photo = (h * scin.density * scin.dedx * scin.light_yield *
                             sis.detection_eff * sis.geometric_eff)
                    new.append(photo)
            data_per_faccia.append(new)
        go=Plot.Drawfacce(data_per_faccia)

    a = Plot.Draw(muon_df)
    
    disegna = [hist_carica0r, hist_caricar, hist_carica0l, hist_carical]
    for graph in disegna:
        a = Plot.drawroot(graph)
    
    return (disegna)
