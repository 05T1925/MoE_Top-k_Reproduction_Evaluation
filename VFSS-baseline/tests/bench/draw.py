# 不同叉树的选择

import matplotlib.pyplot as plt

# =========================
# Benchmark data
# =========================
tree_types = ["Binary", "Quad", "Oct"]

offline_time = [1560.311, 1637.775, 2217.382]
online_time = [842.275, 423.615, 294.346]
total_time = [2402.586, 2061.390, 2511.728]

# =========================
# Create figure
# =========================
plt.figure(figsize=(8.5, 6))

# Offline
plt.plot(
    tree_types,
    offline_time,
    marker="o",
    markersize=10,
    linewidth=2.5,
    color="#E64B35",
    label="Offline"
)

# Online
plt.plot(
    tree_types,
    online_time,
    marker="s",
    markersize=10,
    linewidth=2.5,
    color="#4DBBD5",
    label="Online"
)

# Total
plt.plot(
    tree_types,
    total_time,
    marker="^",
    markersize=11,
    linewidth=2.5,
    color="#00A087",
    label="Offline + Online"
)

# =========================
# Add numerical labels
# =========================
for i, value in enumerate(offline_time):
    plt.annotate(
        f"{value:.1f}",
        (i, value),
        xytext=(0, 12),
        textcoords="offset points",
        ha="center",
        fontsize=14
    )

for i, value in enumerate(online_time):
    plt.annotate(
        f"{value:.1f}",
        (i, value),
        xytext=(0, -24),
        textcoords="offset points",
        ha="center",
        fontsize=14
    )

for i, value in enumerate(total_time):
    plt.annotate(
        f"{value:.1f}",
        (i, value),
        xytext=(0, 12),
        textcoords="offset points",
        ha="center",
        fontsize=14
    )

# =========================
# Axis settings
# =========================
plt.xlabel("Tree Arity", fontsize=20)
plt.ylabel("Time (ms)", fontsize=20)

plt.xticks(fontsize=18)
plt.yticks(fontsize=18)

# =========================
# Legend
# =========================
plt.legend(
    fontsize=17,
    loc="best",
    frameon=True
)

# =========================
# Grid
# =========================
plt.grid(
    True,
    linestyle="--",
    linewidth=0.8,
    alpha=0.5
)

plt.tight_layout()

# =========================
# Save figures
# =========================
plt.savefig(
    "tree_performance_comparison.png",
    dpi=300,
    bbox_inches="tight"
)

plt.savefig(
    "tree_performance_comparison.pdf",
    bbox_inches="tight"
)

plt.show()