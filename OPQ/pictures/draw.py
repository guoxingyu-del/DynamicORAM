import matplotlib.pyplot as plt
import math

from matplotlib.ticker import FormatStrFormatter, MaxNLocator

x = []
time_dynamic_path = []
bandwith_dynamic_path = []
stash_dynamic_path = []

time_dynamic_circuit = []
bandwith_dynamic_circuit = []
stash_dynamic_circuit = []

time_normal_path = []
bandwith_normal_path = []
stash_normal_path = []

time_normal_circuit = []
bandwith_normal_circuit = []
stash_normal_circuit = []

time_insecure_pq = []
bandwith_insecure_pq = []


def clear_data():
    x.clear()
    time_dynamic_path.clear()
    bandwith_dynamic_path.clear()
    stash_dynamic_path.clear()

    time_dynamic_circuit.clear()
    bandwith_dynamic_circuit.clear()
    stash_dynamic_circuit.clear()

    time_normal_path.clear()
    bandwith_normal_path.clear()
    stash_normal_path.clear()

    time_normal_circuit.clear()
    bandwith_normal_circuit.clear()
    stash_normal_circuit.clear()

    time_insecure_pq.clear()
    bandwith_insecure_pq.clear()

choice_str = ["Insert", "Extract"]

def read_data(choice):
    flag = False
    if choice == 2:
        flag = True
    tmp_str = choice_str[choice - 1]

    with open('../results/PathOPQ' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()
        if flag:
            lines.reverse()
        for line_number, line in enumerate(lines, start=2):
            line = line[:-1]
            res = line.split(' ')
            sum_bandwith += int(res[0])
            sum_time += int(res[1])
            max_stash = max(max_stash, int(res[2]))
            if line_number == (1 << index):
                x.append(line_number)
                bandwith_normal_path.append(float(sum_bandwith/(line_number-1)/1024))
                time_normal_path.append(float(sum_time/(line_number-1)))
                # stash_normal_path.append(float(max_stash/72)+0.04)
                stash_normal_path.append(float(max_stash/72))
                index += 4

    with open('../results/CircuitOPQ' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()
        if flag:
            lines.reverse()
        for line_number, line in enumerate(lines, start=2):
            line = line[:-1]
            res = line.split(' ')
            sum_bandwith += int(res[0])
            sum_time += int(res[1])
            max_stash = max(max_stash, int(res[2]))
            if line_number == (1 << index):
                bandwith_normal_circuit.append(float(sum_bandwith/(line_number-1)/1024))
                time_normal_circuit.append(float(sum_time/(line_number-1)))
                # stash_normal_circuit.append(float(max_stash/72)-0.04)
                stash_normal_circuit.append(float(max_stash/72))
                index += 4

    with open('../results/DynamicPathOPQ' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()
        if flag:
            lines.reverse()
        for line_number, line in enumerate(lines, start=2):
            line = line[:-1]
            res = line.split(' ')
            sum_bandwith += int(res[0])
            sum_time += int(res[1])
            max_stash = max(max_stash, int(res[2]))
            if line_number == (1 << index):
                bandwith_dynamic_path.append(float(sum_bandwith/(line_number-1)/1024))
                time_dynamic_path.append(float(sum_time/(line_number-1)))
                # stash_dynamic_path.append(float(max_stash/72)-0.08)
                stash_dynamic_path.append(float(max_stash/72))
                index += 4

    with open('../results/DynamicCircuitOPQ' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()
        if flag:
            lines.reverse()
        for line_number, line in enumerate(lines, start=2):
            line = line[:-1]
            res = line.split(' ')
            sum_bandwith += int(res[0])
            sum_time += int(res[1])
            max_stash = max(max_stash, int(res[2]))
            if line_number == (1 << index):
                bandwith_dynamic_circuit.append(float(sum_bandwith/(line_number-1)/1024))
                time_dynamic_circuit.append(float(sum_time/(line_number-1)))
                # stash_dynamic_circuit.append(float(max_stash/72)+0.08)
                stash_dynamic_circuit.append(float(max_stash/72))
                index += 4

    with open('../results/InsecurePQ' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        index = 4
        lines = f.readlines()
        if flag:
            lines.reverse()
        for line_number, line in enumerate(lines, start=2):
            line = line[:-1]
            res = line.split(' ')
            sum_bandwith += int(res[0])
            sum_time += int(res[1])
            if line_number == (1 << index):
                bandwith_insecure_pq.append(float(sum_bandwith/(line_number-1)/1024))
                time_insecure_pq.append(float(sum_time/(line_number-1)))
                index += 4

def __draw(choice):
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 3.5))
    labels = [4, 8, 12, 16, 20]

    ax1.set_xscale("log", base=2)
    ax1.set_yscale("log")
    # ax1.set_ylim(top=100)

    ax1.plot(x, bandwith_normal_path, label='path', color='lime', marker='o', linestyle='--', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_normal_circuit, label='circuit', color='#FF0000', marker='v', linestyle='--', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_dynamic_path, label='d-path', color='blue', marker='o', linestyle=':', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_dynamic_circuit, label='d-circuit', color='orange', marker='v', linestyle=':', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_insecure_pq, label='insecure', color='#808080', marker='d', linestyle=':', markersize=12, linewidth=4)
    ax1.set_xlabel('# of elements (64 bytes)', fontsize=18)
    ax1.set_ylabel('Bandwith (KB)', fontsize=18)
    ax1.set_xticks(ticks=x, labels=[rf'$2^{{{i}}}$' for i in labels])
    ax1.tick_params(axis='both', which='major', labelsize=18)

    ax1.grid(True)
    ax1.set_ylim(bottom=0.1)

    ax2.set_xscale("log", base=2)
    ax2.set_yscale("log")
    # ax2.set_ylim(top=100)
   
    ax2.plot(x, time_normal_path, label='S-Path OPQ', color='lime', marker='o', linestyle='--', markersize=12, linewidth=4)
    ax2.plot(x, time_normal_circuit, label='S-Circuit OPQ', color='#FF0000', marker='v', linestyle='--', markersize=12, linewidth=4)
    ax2.plot(x, time_dynamic_path, label='D-Path OPQ', color='blue', marker='o', linestyle=':', markersize=12, linewidth=4)
    ax2.plot(x, time_dynamic_circuit, label='D-Circuit OPQ', color='orange', marker='v', linestyle=':', markersize=12, linewidth=4)
    ax2.plot(x, time_insecure_pq, label='Insecure PQ', color='#808080', marker='d', linestyle=':', markersize=12, linewidth=4)

    ax2.set_xlabel('# of elements (64 bytes)', fontsize=18)
    ax2.set_ylabel('Time (us)', fontsize=18)
    ax2.set_xticks(ticks=x, labels=[rf'$2^{{{i}}}$' for i in labels])
    ax2.tick_params(axis='both', which='major', labelsize=18) 
    ax2.grid(True)
    
    ax2.legend(
        loc='upper center',         
        bbox_to_anchor=(0.5, 1.35),  
        ncol=6,                      
        frameon=True,               
        fontsize=18
    )

    ax3.set_xscale("log", base=2)

    ax3.plot(x, stash_normal_path, label='Path', color='lime', marker='o', linestyle='--', markersize=12, linewidth=4)
    ax3.plot(x, stash_normal_circuit, label='Circuit', color='#FF0000', marker='v', linestyle='--', markersize=12, linewidth=4)
    ax3.plot(x, stash_dynamic_path, label='D-Path', color='blue', marker='o', linestyle=':', markersize=12, linewidth=4)
    ax3.plot(x, stash_dynamic_circuit, label='D-Circuit', color='orange', marker='v', linestyle=':', markersize=12, linewidth=4)
    ax3.set_xlabel('# of elements (64 bytes)', fontsize=18)
    ax3.set_ylabel('Stash (# of elements)', fontsize=18)
    ax3.set_xticks(ticks=x, labels=[rf'$2^{{{i}}}$' for i in labels])
    ax3.tick_params(axis='both', which='major', labelsize=18) 
    ax3.grid(True)
    # ax3.set_ylim(top=3, bottom=-1)
    # ax3.set_ylim(top=15)

    plt.subplots_adjust(wspace=0.4, top=0.8)

    plt.savefig(choice_str[choice - 1] + '.pdf', bbox_inches='tight', dpi=300)

    plt.show()

def display_menu():
    print("Choose(1-2): ")
    print("1. Insert bandwidth, time, stash size")
    print("2. Extract bandwidth, time, stash siz")
    print("0. Exit")


def main():
    display_menu()
    while True:
        choice = int(input())
        if choice == 0:
            break
        clear_data()
        read_data(choice)
        __draw(choice)
            

if __name__ == "__main__":
    main()