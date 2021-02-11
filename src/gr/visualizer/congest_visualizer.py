import numpy as np
import math
import matplotlib.pyplot as plt
import subprocess, os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--input_folder', 
                    type=str, help="ripple_log_folder", 
                    default="/data/ssd/lxliu/ripple/bin/ripple_2020_06_03_19_45_25/")
parser.add_argument('--output_heatmap', type=str, default="congestion_heatmap.png")
args = parser.parse_args()

def cugr_visualizer(inputFile, outputFile):
    num_layers = 0
    gcell_x = 0
    gcell_y = 0
    congestion_map = None
    x_index = 0
    y_index = 0
    z_index = 0
    lineCounter = 0
    print("\n------- Get current def's cugr_congestion.txt -------")
    print("Current input: %s" % inputFile)
    with open(inputFile, "r") as f:
        lines = f.readlines()
        for line in lines:
            if(line[0] == "#"):
                strs = line.split(" ")
                if(strs[1]=="layer:"):
                    num_layers = int(strs[2])
                if(strs[1]=="gcell(x):"):
                    gcell_x = int(strs[2])
                if(strs[1]=="gcell(y):"):
                    gcell_y = int(strs[2])
                    congestion_map = np.zeros((gcell_x, gcell_y, 2))
                    demand_map = np.zeros((gcell_x, gcell_y, num_layers))
                    supply_map = np.zeros((gcell_x, gcell_y, num_layers))
                continue
            else:
                x_index = lineCounter % gcell_x
                y_index = 0
                z_index = lineCounter // gcell_x
                line = line.replace(", ", ",")
                tuples = line.split(" ")
                for three_element_items in tuples:
                    three_element_items = three_element_items.replace("(","")
                    three_element_items = three_element_items.replace(")","")
                    items = three_element_items.split(",")
                    # congestion = float(items[0]) - float(items[1]) - 1.5 * math.sqrt(float(items[2]))
                    # congestion_map[x_index, y_index, z_index] = -1.0 * min(congestion, 0.0)
                    demand_map[x_index, y_index, z_index] = float(items[1]) + 1.5 * math.sqrt(float(items[2]))
                    supply_map[x_index, y_index, z_index] = float(items[0])
                    y_index += 1
                lineCounter += 1

    congestion_map[:,:,0] = np.divide(np.sum(demand_map[:,:,0::2], axis=2), np.sum(supply_map[:,:,0::2], axis=2))
    congestion_map[:,:,1] = np.divide(np.sum(demand_map[:,:,1::2], axis=2), np.sum(supply_map[:,:,1::2], axis=2))

    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(21, 10))
    for i, ax in enumerate(axes.flat):
        if i == 0:
            im = ax.imshow(np.transpose(congestion_map[:,:,0]), cmap='viridis', origin='lower')
            ax.title.set_text("Congestion Map of H/V")
        else:
            im = ax.imshow(np.transpose(congestion_map[:,:,1]), cmap='viridis', origin='lower')
            ax.title.set_text("Congestion Map of H/V")
        ax.set_xticks([], [])
        ax.set_xticks([], [])
        fig.colorbar(im, ax=ax)

    print("Storing the heatmap in %s" % os.path.join(os.getcwd(), outputFile))
    plt.savefig(os.path.join(os.getcwd(), outputFile))

    print("\n------- Finishing drawing congestion heatmap -------")

def prob_visualizer(inputFile, outputFile):
    num_layers = 0
    gcell_x = 0
    gcell_y = 0
    congestion_map = None
    x_index = 0
    y_index = 0
    z_index = 0
    lineCounter = 0
    print("\n------- Get current def's prob_congestion.txt -------")
    print("Current input: %s" % inputFile)
    with open(inputFile, "r") as f:
        lines = f.readlines()
        for line in lines:
            if(line[0] == "#"):
                strs = line.split(" ")
                if(strs[1]=="layer:"):
                    num_layers = int(strs[2])
                if(strs[1]=="gcell(x):"):
                    gcell_x = int(strs[2])
                if(strs[1]=="gcell(y):"):
                    gcell_y = int(strs[2])
                    congestion_map = np.zeros((gcell_x, gcell_y, num_layers))
                continue
            else:
                z_index = lineCounter
                str_xys = line.split("\t")
                for x_index, str_xy in enumerate(str_xys):
                    str_ys = str_xy.split(" ")
                    for y_index, str_y in enumerate(str_ys):
                        items = str_y.split(",")
                        supplyValue = float(items[0])
                        demandValue = float(items[1]) 
                        congestion = demandValue / supplyValue
                        congestion_map[x_index, y_index, z_index] = congestion
                lineCounter += 1

    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(10, 5))
    for i, ax in enumerate(axes.flat):
        if i == 0:
            im = ax.imshow(np.transpose(congestion_map[:,:,0]), cmap='viridis', origin='lower')
            ax.title.set_text("Congestion Map of H/V")
        else:
            im = ax.imshow(np.transpose(congestion_map[:,:,1]), cmap='viridis', origin='lower')
            ax.title.set_text("Congestion Map of H/V")
        ax.set_xticks([], [])
        ax.set_xticks([], [])
        fig.colorbar(im, ax=ax)

    print("Storing the heatmap in %s" % os.path.join(os.getcwd(), outputFile))
    plt.savefig(os.path.join(os.getcwd(), outputFile))

    print("\n------- Finishing drawing congestion heatmap -------")
    plt.close()

def visualizer_main(input_folder, output_heatmap, gr_type):
    inputFileName = "%s_congestion.txt" % gr_type
    outputHeatmapFileName = "%s_%s" % (gr_type, output_heatmap)
    benchmark_root = input_folder
    allBmName = [o for o in os.listdir(benchmark_root)  if os.path.isdir(os.path.join(benchmark_root,o))]
    allBmName = [o for o in allBmName if "_%s" % (gr_type) in o]

    inputFiles = [os.path.join(benchmark_root, bmName, inputFileName) for bmName in allBmName]
    outputFiles = [os.path.join(benchmark_root, bmName, outputHeatmapFileName) for bmName in allBmName]
    if gr_type == "prob":
        for idx, inputFile in enumerate(inputFiles):
            outputFile = outputFiles[idx]
            prob_visualizer(inputFile, outputFile)
    elif gr_type == "cugr":
        for idx, inputFile in enumerate(inputFiles):
            outputFile = outputFiles[idx]
            cugr_visualizer(inputFile, outputFile)
    else:
        raise NotImplementedError("Gr_type should be [cugr] or [prob]")

if __name__ == "__main__":
    gr_types = ["prob", "cugr"]
    for gr_type in gr_types:
        print("------------ Start visualizing %s's log files ------------" % gr_type)
        visualizer_main(args.input_folder, args.output_heatmap, gr_type)