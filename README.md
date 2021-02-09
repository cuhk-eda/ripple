# Ripple
Ripple is a VLSI placement tool developed by the research team supervised by Prof. Evangeline F.Y. Young in The Chinese University of Hong Kong (CUHK).  
Circuits with increasing numbers of cells of multi-row height have brought challenges to traditional placers on efficiency and effectiveness.
Besides providing an overlap-free solution close to the global placement (GP) solution, constraints on power and ground (P/G) alignments, fence region, and routability (e.g., edge spacing, pin short/inaccessible) should be considered.
To tackle the challenges, we design and implement several efficient and effective data structures and algorithms under a holistic framework:
* a legalization method for mixed-cell-height circuits by a window-based cell insertion technique,
* an iterative network-flow-based maximum displacement optimizations,
* a dual-network-flow-based fixed-row-and-order optimization,
* ...

More details are in the following papers:
* Xu He, Tao Huang, Linfu Xiao, Haitong Tian, Guxin Cui, Evangeline F.Y. Young, "[Ripple: An Effective Routability-Driven Placer by Iterative Cell Movement](https://doi.org/10.1109/ICCAD.2011.6105308)", IEEE/ACM International Conference on Computer-Aided Design (ICCAD), San Jose, CA, USA, Nov. 7 - Nov. 10, 2011.
* Xu He, Tao Huang, Wing-Kai Chow, Jian Kuang, Ka-Chun Lam, Wenzan Cai, Evangeline F.Y. Young, "[Ripple 2.0: High Quality Routability-Driven Placement via Global Router Integration](https://doi.org/10.1145/2463209.2488922)", ACM/IEEE Design Automation Conference (DAC), Austin, TX, USA, May 29 - June 7, 2013.
* Wing-Kai Chow, Chak-Wa Pui, Evangeline F.Y. Young, "[Legalization Algorithm for Multiple-Row Height Standard Cell Design](https://doi.org/10.1145/2897937.2898038)", ACM/EDAC/IEEE Design Automation Conference (DAC), Austin, TX, USA, June 5 - June 9, 2016.
* Haocheng Li, Wing-Kai Chow, Gengjie Chen, Evangeline F.Y. Young, Bei Yu, "[Routability-Driven and Fence-Aware Legalization for Mixed-Cell-Height Circuits](https://doi.org/10.1145/3195970.3196107)", ACM/EDAC/IEEE Design Automation Conference (DAC), San Francisco, CA, USA, June 24 - June 29, 2018.
* Haocheng Li, Wing-Kai Chow, Gengjie Chen, Bei Yu, Evangeline F.Y. Young, "[Pin-Accessible Legalization for Mixed-Cell-Height Circuits](https://doi.org/10.1109/TCAD.2021.3053223)", IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems (TCAD), 2021.

## 1. How to Build

**Step 1:** Download the source codes. For example,
~~~bash
$ git clone https://github.com/cuhk-eda/ripple
~~~

**Step 2:** Go to the project root and build by
~~~bash
$ cd ripple/src
$ make mode=release
~~~
