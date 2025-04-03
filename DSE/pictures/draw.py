import matplotlib.pyplot as plt

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

choice_str = ["Addinspec", "Delinspec", "Accessinspec"]

def read_data(choice):
    flag = False
    if choice == 2:
        flag = True
    tmp_str = choice_str[choice - 1]

    with open('../results/PathDSE' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()

        if choice == 3:
            line_number = 1
            for line in lines:
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == 2**index:
                    x.append(line_number)
                    bandwith_normal_path.append(sum_bandwith/line_number/1024)
                    time_normal_path.append(sum_time/line_number)
                    stash_normal_path.append(max_stash/16)
                    line_number = 1
                    sum_bandwith = 0
                    sum_time = 0
                    index += 4
                else:
                    line_number += 1

        else:
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
                    bandwith_normal_path.append(sum_bandwith/(line_number - 1)/1024)
                    time_normal_path.append(sum_time/(line_number - 1))
                    stash_normal_path.append(max_stash/16)
                    index += 4

    with open('../results/CircuitDSE' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()

        if choice == 3:
            line_number = 1
            for line in lines:
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == 2**index:
                    bandwith_normal_circuit.append(sum_bandwith/line_number/1024)
                    time_normal_circuit.append(sum_time/line_number)
                    stash_normal_circuit.append(max_stash/16)
                    line_number = 1
                    sum_time = 0
                    sum_bandwith = 0
                    index += 4
                else:
                    line_number += 1

        else:
            if flag:
                lines.reverse()
            for line_number, line in enumerate(lines, start=2):
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == (1 << index):
                    bandwith_normal_circuit.append(sum_bandwith/(line_number - 1)/1024)
                    time_normal_circuit.append(sum_time/(line_number - 1))
                    stash_normal_circuit.append(max_stash/16)
                    index += 4

    with open('../results/DynamicPathDSE' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()

        if choice == 3:
            line_number = 1
            for line in lines:
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == 2**index:
                    bandwith_dynamic_path.append(sum_bandwith/line_number/1024)
                    time_dynamic_path.append(sum_time/line_number)
                    stash_dynamic_path.append(max_stash/16)
                    line_number = 1
                    sum_bandwith = 0
                    sum_time = 0
                    index += 4
                else:
                    line_number += 1

        else:
            if flag:
                lines.reverse()
            for line_number, line in enumerate(lines, start=2):
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == (1 << index):
                    bandwith_dynamic_path.append(sum_bandwith/(line_number - 1)/1024)
                    time_dynamic_path.append(sum_time/(line_number - 1))
                    stash_dynamic_path.append(max_stash/16)
                    index += 4

    with open('../results/DynamicCircuitDSE' + tmp_str + '.txt', 'r') as f:
        sum_time = 0
        sum_bandwith = 0
        max_stash = 0
        index = 4
        lines = f.readlines()

        if choice == 3:
            line_number = 1
            for line in lines:
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == 2**index:
                    bandwith_dynamic_circuit.append(sum_bandwith/line_number/1024)
                    time_dynamic_circuit.append(sum_time/line_number)
                    stash_dynamic_circuit.append(max_stash/16)
                    line_number = 1
                    sum_time = 0
                    sum_bandwith = 0
                    index += 4
                else:
                    line_number += 1

        else:
            if flag:
                lines.reverse()
            for line_number, line in enumerate(lines, start=2):
                line = line[:-1]
                res = line.split(' ')
                sum_bandwith += int(res[0])
                sum_time += int(res[1])
                max_stash = max(max_stash, int(res[2]))
                if line_number == (1 << index):
                    bandwith_dynamic_circuit.append(sum_bandwith/(line_number - 1)/1024)
                    time_dynamic_circuit.append(sum_time/(line_number - 1))
                    stash_dynamic_circuit.append(max_stash/16)
                    index += 4


def __draw(choice):
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 3.5))
    labels = [4, 8, 12, 16]

    ax1.set_xscale("log", base=2)
    ax1.set_yscale("log")
    # ax1.set_ylim(top=1000)

    ax1.plot(x, bandwith_normal_path, label='path', color='lime', marker='o', linestyle='--', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_normal_circuit, label='circuit', color='#FF0000', marker='v', linestyle='--', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_dynamic_path, label='d-path', color='blue', marker='o', linestyle=':', markersize=12, linewidth=4)
    ax1.plot(x, bandwith_dynamic_circuit, label='d-circuit', color='orange', marker='v', linestyle=':', markersize=12, linewidth=4)
    ax1.set_xlabel('# of elements (16 bytes)', fontsize=18)
    ax1.set_ylabel('Bandwith (KB)', fontsize=18)
    ax1.set_xticks(ticks=x, labels=[rf'$2^{{{i}}}$' for i in labels])
    ax1.tick_params(axis='both', which='major', labelsize=18)

    ax1.grid(True)

    ax2.set_xscale("log", base=2)
    # ax2.set_yscale("log")
    # ax2.set_ylim(bottom=10)

    ax2.plot(x, time_normal_path, label='S-Path DSE', color='lime', marker='o', linestyle='--', markersize=12, linewidth=4)
    ax2.plot(x, time_normal_circuit, label='S-Circuit DSE', color='#FF0000', marker='v', linestyle='--', markersize=12, linewidth=4)
    ax2.plot(x, time_dynamic_path, label='D-Path DSE', color='blue', marker='o', linestyle=':', markersize=12, linewidth=4)
    ax2.plot(x, time_dynamic_circuit, label='D-Circuit DSE', color='orange', marker='v', linestyle=':', markersize=12, linewidth=4)
    ax2.set_xlabel('# of elements (16 bytes)', fontsize=18)
    ax2.set_ylabel('Time (us)', fontsize=18)
    ax2.set_xticks(ticks=x, labels=[rf'$2^{{{i}}}$' for i in labels])
    ax2.tick_params(axis='both', which='major', labelsize=18) 

    # ax2.yaxis.set_major_formatter(FormatStrFormatter("%d"))
    ax2.grid(True)
    
    ax2.legend(
        loc='upper center',
        bbox_to_anchor=(0.5, 1.30), 
        ncol=4,
        frameon=True,
        prop={
            'size': 18, 
        },
    )

    ax3.set_xscale("log", base=2)

    ax3.plot(x, stash_normal_path, label='Path', color='lime', marker='o', linestyle='--', markersize=12, linewidth=4)
    ax3.plot(x, stash_normal_circuit, label='Circuit', color='#FF0000', marker='v', linestyle='--', markersize=12, linewidth=4)
    ax3.plot(x, stash_dynamic_path, label='D-Path', color='blue', marker='o', linestyle=':', markersize=12, linewidth=4)
    ax3.plot(x, stash_dynamic_circuit, label='D-Circuit', color='orange', marker='v', linestyle=':', markersize=12, linewidth=4)

    ax3.set_xlabel('# of elements (16 bytes)', fontsize=18)
    ax3.set_ylabel('Stash (# of elements)', fontsize=18)
    ax3.set_xticks(ticks=x, labels=[rf'$2^{{{i}}}$' for i in labels])
    ax3.tick_params(axis='both', which='major', labelsize=18) 
    ax3.set_ylim(top=15)
    ax3.grid(True)
    
    plt.subplots_adjust(wspace=0.4, top=0.8)

    plt.savefig("DSE" + choice_str[choice - 1] + '.pdf', bbox_inches='tight', dpi=300)

    plt.show()

def display_menu():
    print("Choose(1-3): ")
    print("1. ADD bandwidth, time, stash size")
    print("2. DEL bandwidth, time, stash size")
    print("3. ACCESS bandwidth, time, stash size")
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