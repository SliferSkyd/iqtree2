import matplotlib.pyplot as plt
import numpy as np

# Data
loci = list(range(1, 11))
psi_partition = [3.2, 8.2, 13, 12, 16.2, 11, 12, 14, 13, 14.4]
g_partition = [3.6, 7.8, 10.2, 8.8, 12.6, 9, 8, 11, 10.2, 10.8]
h_partition = [4, 7, 9.2, 6.4, 10.4, 7.4, 5.4, 8.4, 8, 9.8]

# Bar positions
x = np.arange(len(loci))
width = 0.25 # Width of each bar

# Plot
plt.figure(figsize=(12, 6))
plt.bar(x - width, psi_partition, width=width, label='PsiPartition')
plt.bar(x, g_partition, width=width, label='gPartition')
plt.bar(x + width, h_partition, width=width, label='hPartition')

# Labels and Title
plt.xlabel('Loci', fontsize=20)
plt.ylabel('RF Distance', fontsize=20)
plt.title('Robinson-Foulds Distance Across Loci (DNA Simulated 1)', fontsize=24)
plt.xticks(x, loci, fontsize=16)
plt.yticks(fontsize=16)
plt.legend(fontsize=16)
plt.grid(True, axis='y', linestyle='--', alpha=0.7)

# Show plot
plt.tight_layout()
plt.show()