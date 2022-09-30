script = "startscript_ilp.sh"
graphsize = "small"


import os
for radius in range(2, 6):
    FOLDERHERE = f"/home1/adobler/Turbocharging-Heuristics-for-Weak-Coloring-Numbers/graphs/{graphsize}"
    for filename in list(os.listdir(FOLDERHERE)):
        FOLDER = f"/home1/adobler/Turbocharging-Heuristics-for-Weak-Coloring-Numbers/graphs/{graphsize}"
        cmd = ' '.join(["qsub", "-N", f"{filename}_{radius}", f"-l h_vmem=64G -l bc5 -l mem_free=64G -r y -pe pthreads 1 -e ~/error.log -o /home1/adobler/Turbocharging-Heuristics-for-Weak-Coloring-Numbers/outilp/{radius}_{filename}.txt", f"{script}", "-g", f"graphs/{graphsize}/{filename}", "-r", str(radius)])
        # print(cmd)
        os.system(cmd)