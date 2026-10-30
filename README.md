# NS-3 Network Simulation Practicals – DCN Lab 

Hi, I'm Anand. I'm also a third-year student, and I’m putting these NS-3 practicals together for anyone working through the Data Communication and Networking (DCN) lab at BVCOE or elsewhere in GGSIPU.

## A note from one student to another

I’ll be honest: I’m not very familiar with Linux either, and getting NS-3 set up was a bit of a struggle for me. The installation, dependencies, and compilation took time, and I figured other students might run into the same issue.

So I'm sharing both the **C++ source files (`.cc`)** and **Google Colab versions** of the practicals. If you are comfortable using Linux, you can use the source files directly. If, like me, you would rather avoid spending hours sorting out a Linux setup, you can use the Colab versions and run the simulations in your browser.

A quick heads-up: the **first NS-3 setup and build on Google Colab can take around an hour**. It needs to download, configure, and compile NS-3. The time can vary depending on the runtime, so let the first setup finish. Once it is built, running the individual practicals should be much quicker.

I also know that going through the BVCOE DCN lab manual and preparing the lab file can be a hectic task on its own. So, if you need the lab manual as a reference, here is a link that may help:

[DCN Lab Manual – Studocu](https://www.studocu.com/in/document/bharati-vidyapeeths-college-of-engineering/data-communication-and-networks/dcn-lab-manual/39901772?sid=1db7bbc5-fac7-44b3-ba21-308642cd3ea41793381432)

I'm sharing these as a student resource, not as an official BVCOE or GGSIPU publication. Hopefully, they save someone else a bit of setup trouble. Do go through the code as well; it helps when you have to explain the experiment in the lab or during the viva.

## Practicals included

| Practical | Topic |
| --- | --- |
| 2 | NS-3 Installation and Configuration |
| 3 | UDP Echo Application over Point-to-Point Network |
| 4 | P2P and CSMA Network Topology |
| 5 | Random Early Detection (RED) Queue Discipline |
| 6 | Wi-Fi Rate Adaptation |
| 7 | Dumbbell Network Topology – TCP and UDP |
| 8 | Ad Hoc Wireless Network |
| 9 | UDP Data Transfer |

## Getting started

### Option 1: Google Colab

1. Download the required practical code or open the corresponding Colab version.
2. Open [Google Colab](https://colab.research.google.com/drive/15dieVFkxLZY4grNFRZ-WrynermWUIgyF?usp=sharing).
3. Create a new notebook, if needed.
4. Copy the practical code into a code cell.
5. Run the NS-3 setup cell if it has not already been installed in the runtime.
6. Allow around an hour for the initial NS-3 build. Once it is complete, run the practical's simulation cell.

### Option 2: Linux

If NS-3 is already installed on your Linux system:

1. Copy the `.cc` file into the `scratch` directory of your NS-3 installation.
2. Open a terminal in the NS-3 directory.
3. Run:

```bash
./ns3 run scratch/filename
```

Replace `filename` with the source filename, without the `.cc` extension.

## What's included

- **C++ source files:** Implementations of the practicals.
- **Google Colab versions:** Browser-based versions for students who do not have a Linux setup ready.
- **Simulation outputs:** Text output generated during execution, where available.
- **Network topology diagrams:** Diagrams showing the nodes and their connections.
- **PCAP files:** Packet captures for practicals where generated.
- **CSV/XML files:** Simulation data and FlowMonitor outputs, where available.

## A few things to keep in mind

- These files are shared for learning and academic reference.
- You can change the simulation parameters and observe how the network behaves.
- Results may vary with changes in parameters, NS-3 versions, or execution environments.
- The first Colab setup and build can take around an hour; later runs are usually quicker.
- Follow your faculty's instructions and lab manual for the final submission.

If you notice an error or find a better way to implement something, feel free to make improvements and share them.

I hope this helps someone else get through the NS-3 setup and DCN lab work a little more easily.

**— Anand**