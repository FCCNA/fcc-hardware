import matplotlib.pyplot as plt
import mpl_toolkits.mplot3d as a3
from numpy import array, where
from ROOT import TCanvas

def drawroot(grafico):
    canvas = TCanvas()
    canvas.cd()
    grafico.DrawClone()
    canvas.Draw()
    return(0)

def Draw(muon_df):
    #Disegno istogrammi con i dati ottenuti
    scint_listr = (muon_df.query('measure'))['scintr'].tolist()
    scint_listl = (muon_df.query('measure'))['scintl'].tolist()
    chere_listr = (muon_df.query('measure'))['cherer'].tolist()
    chere_listl = (muon_df.query('measure'))['cherel'].tolist()

    plt.figure(figsize=(8, 6))
    plt.hist(scint_listr, bins=100, color='blue', alpha=0.7)
    plt.xlabel('Photons')
    plt.ylabel('Frequency')
    plt.title('Histogram of right scintillation Photons')
    plt.grid(True)
    plt.show()

    plt.figure(figsize=(8, 6))
    plt.hist(chere_listr, bins=100, color='blue', alpha=0.7)
    plt.xlabel('Photons')
    plt.ylabel('Frequency')
    plt.title('Histogram of right chere Photons')
    plt.grid(True)
    plt.show()

    plt.figure(figsize=(8, 6))
    plt.hist(scint_listl, bins=100, color='blue', alpha=0.7)
    plt.xlabel('Photons')
    plt.ylabel('Frequency')
    plt.title('Histogram of left scintillation Photons')
    plt.grid(True)
    plt.show()

    plt.figure(figsize=(8, 6))
    plt.hist(chere_listl, bins=100, color='blue', alpha=0.7)
    plt.xlabel('Photons')
    plt.ylabel('Frequency')
    plt.title('Histogram of left chere Photons')
    plt.grid(True)
    plt.show()

    return(0)

def Drawfacce(data_per_faccia):
    # Disegno istogrammi per tipo di percorso
    name = ['UP-DOWN', 'DOWN-L1', 'DOWN-L2', 'DOWN-L3', 'DOWN-L4', 'UP-L1', 'UP-L2', 'UP-L3', 'UP-L4',
            'L1-L2', 'L1-L3: ', 'L1-L4: ', 'L2-L3', 'L2-L4', 'L3-L4']
    
    for i in range(len(data_per_faccia)):
        plt.figure(figsize=(8, 6))
        plt.hist(data_per_faccia[i], bins=100, color='blue', alpha=0.7)
        plt.xlabel('Photons')
        plt.ylabel('Frequency')
        plt.title(f'Histogram of Photons type {name[i]}')
        plt.grid(True)
        plt.savefig(f"Figures/histogram_photons_type_{name[i]}.png")
        plt.show()
    
    return(0)


def Draw3D(sis, scin, hitmax_measure, x0s, y0s, z0s, us, vs, ws, x1s, y1s, z1s):
    # Rappresentazione 3D
    hitmax_measure_indices = where(hitmax_measure)[0]
    nmax = min(len(hitmax_measure_indices), 100)
    hitmax_measure_indices = hitmax_measure_indices[:nmax]
    fig = plt.figure(figsize=(6, 6), dpi=100)
    ax = fig.add_subplot(111, projection='3d')
    ax.view_init(20, 20)
    ax.quiver(x0s[hitmax_measure_indices], y0s[hitmax_measure_indices],
              z0s[hitmax_measure_indices], us[hitmax_measure_indices],
              vs[hitmax_measure_indices], ws[hitmax_measure_indices],
              length=2, arrow_length_ratio=0, color='darkgreen', alpha=0.4)
    ax.scatter(x1s[hitmax_measure_indices], y1s[hitmax_measure_indices],
               z1s[hitmax_measure_indices], s=4, color='darkgreen')
    ax.set_xlim3d(-0.1, 0.1)
    ax.set_ylim3d(-0.1, 0.1)
    ax.set_zlim3d(0, 0.2)
    plane_vtx1 = array([[- sis.tup_x/2, - sis.tup_y/2, 0],
                           [sis.tup_x/2, -sis.tup_y/2, 0],
                           [sis.tup_x/2, sis.tup_y/2, 0],
                           [-sis.tup_x/2, sis.tup_y/2, 0]])
    plane0 = a3.art3d.Poly3DCollection([plane_vtx1])
    plane0.set_color('#0000ff20')
    plane0.set_edgecolor('#0000ff')
    ax.add_collection3d(plane0)
    plane_vtx2 = array([
        [- sis.tdown_x/2, - sis.tdown_y/2, sis.dt],
        [sis.tdown_x/2, -sis.tdown_y/2, sis.dt],
        [sis.tdown_x/2, sis.tdown_y/2, sis.dt],
        [-sis.tdown_x/2, sis.tdown_y/2, sis.dt]])
    plane1 = a3.art3d.Poly3DCollection([plane_vtx2])
    plane1.set_color('#0000ff20')
    plane1.set_edgecolor('#0000ff')
    ax.add_collection3d(plane1)
    third_plane_vtx1 = array([
        [-scin.x/2, -scin.y/2, sis.tup_scint],
        [scin.x/2, -scin.y/2, sis.tup_scint],
        [scin.x/2, scin.y/2, sis.tup_scint],
        [-scin.x/2, scin.y/2, sis.tup_scint]])
    third_plane_vtx2 = array([
        [-scin.x/2, -scin.y/2, sis.tup_scint+scin.z],
        [scin.x/2, -scin.y/2, sis.tup_scint+scin.z],
        [scin.x/2, scin.y/2, sis.tup_scint+scin.z],
        [-scin.x/2, scin.y/2, sis.tup_scint+scin.z]])
    third_plane1 = a3.art3d.Poly3DCollection([third_plane_vtx1])
    third_plane2 = a3.art3d.Poly3DCollection([third_plane_vtx2])
    third_plane1.set_color('#00ff0020')  
    third_plane1.set_edgecolor('#00ff00')  
    third_plane2.set_color('#00ff0020')  
    third_plane2.set_edgecolor('#00ff00')  
    ax.add_collection3d(third_plane1)
    ax.add_collection3d(third_plane2)    
    vert_plane1_vtx = array([
        [scin.x/2, -scin.y/2, sis.tup_scint],
        [scin.x/2, -scin.y/2, sis.tup_scint + scin.z],
        [scin.x/2, scin.y/2, sis.tup_scint + scin.z],
        [scin.x/2, scin.y/2, sis.tup_scint]])
    vert_plane2_vtx = array([
        [-scin.x/2, -scin.y/2, sis.tup_scint],
        [-scin.x/2, -scin.y/2, sis.tup_scint + scin.z],
        [-scin.x/2, scin.y/2, sis.tup_scint + scin.z],
        [-scin.x/2, scin.y/2, sis.tup_scint]])
    vert_plane1 = a3.art3d.Poly3DCollection([vert_plane1_vtx])
    vert_plane2 = a3.art3d.Poly3DCollection([vert_plane2_vtx])
    vert_plane1.set_color('#ff000020')
    vert_plane1.set_edgecolor('#ff0000')
    vert_plane2.set_color('#ff000020')
    vert_plane2.set_edgecolor('#ff0000')
    ax.add_collection3d(vert_plane1)
    ax.add_collection3d(vert_plane2)
    piano_laterale_1_vtx = array([
        [-scin.x/2, - scin.y/2, sis.tup_scint],
        [scin.x/2, -scin.y/2, sis.tup_scint],
        [scin.x/2, -scin.y/2, sis.tup_scint + scin.z],
        [-scin.x/2, -scin.y/2, sis.tup_scint+ scin.z]])
    piano_laterale_2_vtx = array([
        [-scin.x/2, scin.y/2, sis.tup_scint],
        [scin.x/2, scin.y/2, sis.tup_scint],
        [scin.x/2, scin.y/2, sis.tup_scint + scin.z],
        [-scin.x/2, scin.y/2, sis.tup_scint+ scin.z]])
    piano_laterale_1 = a3.art3d.Poly3DCollection([piano_laterale_1_vtx])
    piano_laterale_2 = a3.art3d.Poly3DCollection([piano_laterale_2_vtx])
    piano_laterale_1.set_color('#ff000020')
    piano_laterale_1.set_edgecolor('#ff0000')
    piano_laterale_2.set_color('#ff000020')
    piano_laterale_2.set_edgecolor('#ff0000')
    ax.add_collection3d(piano_laterale_1)
    ax.add_collection3d(piano_laterale_2)
    
    ax.invert_zaxis()
    plt.savefig("Figures/plot_measure.png")
    plt.show()

    return(0)
