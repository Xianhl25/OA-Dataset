import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, filtfilt, find_peaks
from scipy.interpolate import interp1d

# 读取CSV文件
# 设置文件夹路径
folder_path = "D:/04_SUSTech_Robotics/03_Soft_ExoSkeleton/03_AI_Control/01_IMU/步态测试导出数据"

# 获取文件夹中所有文件
files = os.listdir(folder_path)
# 筛选出所有 .csv 文件
csv_files = [file for file in files if file.endswith('.csv')]
# 读取每个 .csv 文件并存储到列表中
dataframes = []
for csv_file in csv_files:
    file_path = os.path.join(folder_path, csv_file)
    df = pd.read_csv(file_path)
    dataframes.append(df)
# 文件名列表
file_names = [os.path.splitext(file)[0] for file in csv_files]  # 去掉文件扩展名
# 打印文件名和数据框的长度以确认
print("文件名列表：", file_names)
print("数据框列表长度：", len(dataframes))

plt.rcParams['font.sans-serif'] = ['Times New Roman']  # 使用黑体
plt.rcParams['axes.unicode_minus'] = False  # 正确显示负号

def save_for_someone(data, data_name):
    # 检查列名是否存在
    required_columns = [
        "I99", "I13", "I0", "I1", "I9", "I10",  # 左腿
        "I100", "I31", "I18", "I19", "I27", "I28"  # 右腿
    ]
    if not all(col in data.columns for col in required_columns):
        raise ValueError(f"缺少列，请检查列名是否正确！缺少的列有：{[col for col in required_columns if col not in data.columns]}")

    # 提取所需的列数据
    # 左腿
    pressure_left = data["I99"]
    gyro_left = data["I13"]
    roll_thigh_left = data["I0"]
    pitch_thigh_left = data["I1"]
    roll_shank_left = data["I9"]
    pitch_shank_left = data["I10"]

    # 右腿
    pressure_right = data["I100"]
    gyro_right = data["I31"]
    roll_thigh_right = data["I18"]
    pitch_thigh_right = data["I19"]
    roll_shank_right = data["I27"]
    pitch_shank_right = data["I28"]

    # 从足底压力曲线中减去其第一个值
    pressure_left = pressure_left - pressure_left.iloc[0]
    pressure_right = pressure_right - pressure_right.iloc[0]

    # 低通滤波函数
    def low_pass_filter(data, cutoff, fs, order=5):
        nyquist = 0.5 * fs
        normal_cutoff = cutoff / nyquist
        b, a = butter(order, normal_cutoff, btype='low', analog=False)
        y = filtfilt(b, a, data)
        return y

    # 应用低通滤波
    cutoff_frequency = 5  # 低通滤波截止频率（单位：Hz）
    fs = 100  # 采样频率（单位：Hz），假设为100Hz

    # 左腿
    filtered_pressure_left = low_pass_filter(pressure_left, cutoff_frequency, fs)
    filtered_gyro_left = low_pass_filter(gyro_left, cutoff_frequency, fs)
    filtered_roll_thigh_left = low_pass_filter(roll_thigh_left, cutoff_frequency, fs)
    filtered_pitch_thigh_left = low_pass_filter(pitch_thigh_left, cutoff_frequency, fs)
    filtered_roll_shank_left = low_pass_filter(roll_shank_left, cutoff_frequency, fs)
    filtered_pitch_shank_left = low_pass_filter(pitch_shank_left, cutoff_frequency, fs)

    # 右腿
    filtered_pressure_right = low_pass_filter(pressure_right, cutoff_frequency, fs)
    filtered_gyro_right = low_pass_filter(gyro_right, cutoff_frequency, fs)
    filtered_roll_thigh_right = low_pass_filter(roll_thigh_right, cutoff_frequency, fs)
    filtered_pitch_thigh_right = low_pass_filter(pitch_thigh_right, cutoff_frequency, fs)
    filtered_roll_shank_right = low_pass_filter(roll_shank_right, cutoff_frequency, fs)
    filtered_pitch_shank_right = low_pass_filter(pitch_shank_right, cutoff_frequency, fs)

    # 定义步态周期分割函数
    def split_gait_cycle(pressure):
        peaks, _ = find_peaks(pressure)
        return peaks

    # 分割步态周期
    peaks_left = split_gait_cycle(filtered_pressure_left)
    peaks_right = split_gait_cycle(filtered_pressure_right)

    # 确定步态周期的开始和结束，并计算每个周期的Gyro峰值和最小值
    def determine_gait_cycles(peaks, gyro):
        gait_cycles = []
        cycle_gyro_peaks = []
        cycle_gyro_mins = []
        for i in range(1, len(peaks)):
            start = peaks[i-1]
            end = peaks[i]
            cycle_gyro = gyro[start:end]
            peak = np.max(cycle_gyro)
            min_val = np.min(cycle_gyro)
            cycle_gyro_peaks.append(peak)
            cycle_gyro_mins.append(min_val)
            gait_cycles.append((start, end))
        return gait_cycles, cycle_gyro_peaks, cycle_gyro_mins

    # 左腿
    gait_cycles_left, cycle_gyro_peaks_left, cycle_gyro_mins_left = determine_gait_cycles(peaks_left, filtered_gyro_left)
    # 右腿
    gait_cycles_right, cycle_gyro_peaks_right, cycle_gyro_mins_right = determine_gait_cycles(peaks_right, filtered_gyro_right)

    # 计算所有步态周期的Gyro峰值和最小值的平均值
    average_gyro_peak_left = np.mean(cycle_gyro_peaks_left)
    average_gyro_min_left = np.mean(cycle_gyro_mins_left)
    average_gyro_peak_right = np.mean(cycle_gyro_peaks_right)
    average_gyro_min_right = np.mean(cycle_gyro_mins_right)

    # 计算峰值和最小值的上下30%误差带
    error_value = 0.3
    peak_lower_bound_left = average_gyro_peak_left * (1 - error_value)
    peak_upper_bound_left = average_gyro_peak_left * (1 + error_value)
    min_lower_bound_left = average_gyro_min_left * (1 - error_value)
    min_upper_bound_left = average_gyro_min_left * (1 + error_value)

    peak_lower_bound_right = average_gyro_peak_right * (1 - error_value)
    peak_upper_bound_right = average_gyro_peak_right * (1 + error_value)
    min_lower_bound_right = average_gyro_min_right * (1 - error_value)
    min_upper_bound_right = average_gyro_min_right * (1 + error_value)

    # 判断非正常步态周期
    def find_abnormal_cycles(gait_cycles, gyro, peak_lower_bound, peak_upper_bound, min_lower_bound, min_upper_bound):
        abnormal_cycles = [
            (start, end) for start, end in gait_cycles
            if not (peak_lower_bound <= np.max(gyro[start:end]) <= peak_upper_bound) or
            not (min_lower_bound >= np.min(gyro[start:end]) >= min_upper_bound)
        ]
        return abnormal_cycles

    abnormal_cycles_left = find_abnormal_cycles(gait_cycles_left, filtered_gyro_left, peak_lower_bound_left, peak_upper_bound_left, min_lower_bound_left, min_upper_bound_left)
    abnormal_cycles_right = find_abnormal_cycles(gait_cycles_right, filtered_gyro_right, peak_lower_bound_right, peak_upper_bound_right, min_lower_bound_right, min_upper_bound_right)

    # 创建新的数组存储正常的步态周期数据
    def filter_cycles(cycles, abnormal_cycles):
        normal_cycles = [cycle for cycle in cycles if cycle not in abnormal_cycles]
        return normal_cycles

    normal_gait_cycles_left = filter_cycles(gait_cycles_left, abnormal_cycles_left)
    normal_gait_cycles_right = filter_cycles(gait_cycles_right, abnormal_cycles_right)

    # 提取正常步态周期的数据
    def extract_clean_data(data, cycles):
        return np.concatenate([data[start:end] for start, end in cycles])

    filtered_pressure_left_clean = extract_clean_data(filtered_pressure_left, normal_gait_cycles_left)
    filtered_gyro_left_clean = extract_clean_data(filtered_gyro_left, normal_gait_cycles_left)
    filtered_roll_thigh_left_clean = extract_clean_data(filtered_roll_thigh_left, normal_gait_cycles_left)
    filtered_pitch_thigh_left_clean = extract_clean_data(filtered_pitch_thigh_left, normal_gait_cycles_left)
    filtered_roll_shank_left_clean = extract_clean_data(filtered_roll_shank_left, normal_gait_cycles_left)
    filtered_pitch_shank_left_clean = extract_clean_data(filtered_pitch_shank_left, normal_gait_cycles_left)

    filtered_pressure_right_clean = extract_clean_data(filtered_pressure_right, normal_gait_cycles_right)
    filtered_gyro_right_clean = extract_clean_data(filtered_gyro_right, normal_gait_cycles_right)
    filtered_roll_thigh_right_clean = extract_clean_data(filtered_roll_thigh_right, normal_gait_cycles_right)
    filtered_pitch_thigh_right_clean = extract_clean_data(filtered_pitch_thigh_right, normal_gait_cycles_right)
    filtered_roll_shank_right_clean = extract_clean_data(filtered_roll_shank_right, normal_gait_cycles_right)
    filtered_pitch_shank_right_clean = extract_clean_data(filtered_pitch_shank_right, normal_gait_cycles_right)

    # 插值函数
    def interpolate_cycles(data, cycles, num_points=500):
        interpolated_data = []
        for start, end in cycles:
            x = np.arange(start, end)
            y = data[start:end]
            if len(x) != len(y) or len(y) == 0:  # 检查数据是否为空
                raise ValueError(f"x and y length mismatch: len(x)={len(x)}, len(y)={len(y)}")
            f = interp1d(x, y, kind='cubic', fill_value="extrapolate")
            x_new = np.linspace(start, end - 1, num=num_points)  # 插值到固定长度
            interpolated_data.append(f(x_new))
        return interpolated_data

    # 求平均函数
    def average_cycles(interpolated_data):
        return np.mean(interpolated_data, axis=0)

    output_subfolder = os.path.join(folder_path, data_name)
    os.makedirs(output_subfolder, exist_ok=True)
    output_bufferfolder = os.path.join(output_subfolder, 'buffer')
    os.makedirs(output_bufferfolder, exist_ok=True)
    # 可视化窗口1：原始和处理后的数据
    plt.figure(figsize=(15, 8))
    plt.suptitle(f"{data_name}:Origin VS Filter", fontsize=16, fontname='Microsoft YaHei')
    plt.subplot(2, 1, 1)
    plt.plot(pressure_left, label="Left Foot Pressure")
    plt.plot(gyro_left, label="Left Gyro")
    plt.plot(roll_thigh_left, label="Left Thigh Roll")
    plt.plot(pitch_thigh_left, label="Left Thigh Pitch")
    plt.plot(roll_shank_left, label="Left Shank Roll")
    plt.plot(pitch_shank_left, label="Left Shank Pitch")
    plt.plot(pressure_right, label="Right Foot Pressure")
    plt.plot(gyro_right, label="Right Gyro")
    plt.plot(roll_thigh_right, label="Right Thigh Roll")
    plt.plot(pitch_thigh_right, label="Right Thigh Pitch")
    plt.plot(roll_shank_right, label="Right Shank Roll")
    plt.plot(pitch_shank_right, label="Right Shank Pitch")
    plt.legend()
    plt.title("Original Data")

    plt.subplot(2, 1, 2)
    plt.plot(filtered_pressure_left, label="Filtered Left Foot Pressure")
    plt.plot(filtered_gyro_left, label="Filtered Left Gyro")
    plt.plot(filtered_roll_thigh_left, label="Filtered Left Thigh Roll")
    plt.plot(filtered_pitch_thigh_left, label="Filtered Left Thigh Pitch")
    plt.plot(filtered_roll_shank_left, label="Filtered Left Shank Roll")
    plt.plot(filtered_pitch_shank_left, label="Filtered Left Shank Pitch")
    plt.plot(filtered_pressure_right, label="Filtered Right Foot Pressure")
    plt.plot(filtered_gyro_right, label="Filtered Right Gyro")
    plt.plot(filtered_roll_thigh_right, label="Filtered Right Thigh Roll")
    plt.plot(filtered_pitch_thigh_right, label="Filtered Right Thigh Pitch")
    plt.plot(filtered_roll_shank_right, label="Filtered Right Shank Roll")
    plt.plot(filtered_pitch_shank_right, label="Filtered Right Shank Pitch")
    plt.legend()
    plt.title("Filtered Data")
    plt.tight_layout()
    output_image_path = os.path.join(output_subfolder, "buffer/01-Origin_VS_Filter.png")
    plt.savefig(output_image_path)
    plt.close()

    # 可视化窗口2：步态周期分割和异常检测
    plt.figure(figsize=(15, 8))
    plt.suptitle(f"{data_name}:Gait Track and Delete", fontsize=16, fontname='Microsoft YaHei')
    plt.subplot(2, 1, 1)
    plt.plot(filtered_pressure_left, label="Filtered Left Foot Pressure")
    plt.plot(filtered_gyro_left, label="Filtered Left Gyro", color="lightgreen")
    for i, peak in enumerate(peaks_left[:-1]):
        start = peak
        end = peaks_left[i + 1]
        color = 'r' if (start, end) in abnormal_cycles_left else 'k'
        plt.axvline(x=start, color=color, linestyle='--')
        plt.axvline(x=end, color=color, linestyle='--')
    plt.axhline(y=peak_lower_bound_left, color='y', linestyle='--', label=f"Peak Lower Bound ({peak_lower_bound_left:.2f})")
    plt.axhline(y=peak_upper_bound_left, color='y', linestyle='--', label=f"Peak Upper Bound ({peak_upper_bound_left:.2f})")
    plt.axhline(y=min_lower_bound_left, color='c', linestyle='--', label=f"Min Lower Bound ({min_lower_bound_left:.2f})")
    plt.axhline(y=min_upper_bound_left, color='c', linestyle='--', label=f"Min Upper Bound ({min_upper_bound_left:.2f})")
    plt.legend()
    plt.title("Left Leg Gait Cycles")

    plt.subplot(2, 1, 2)
    plt.plot(filtered_pressure_right, label="Filtered Right Foot Pressure")
    plt.plot(filtered_gyro_right, label="Filtered Right Gyro", color="lightgreen")
    for i, peak in enumerate(peaks_right[:-1]):
        start = peak
        end = peaks_right[i + 1]
        color = 'r' if (start, end) in abnormal_cycles_right else 'k'
        plt.axvline(x=start, color=color, linestyle='--')
        plt.axvline(x=end, color=color, linestyle='--')
    plt.axhline(y=peak_lower_bound_right, color='y', linestyle='--', label=f"Peak Lower Bound ({peak_lower_bound_right:.2f})")
    plt.axhline(y=peak_upper_bound_right, color='y', linestyle='--', label=f"Peak Upper Bound ({peak_upper_bound_right:.2f})")
    plt.axhline(y=min_lower_bound_right, color='c', linestyle='--', label=f"Min Lower Bound ({min_lower_bound_right:.2f})")
    plt.axhline(y=min_upper_bound_right, color='c', linestyle='--', label=f"Min Upper Bound ({min_upper_bound_right:.2f})")
    plt.legend()
    plt.title("Right Leg Gait Cycles")
    plt.tight_layout()
    output_image_path = os.path.join(output_subfolder, f"buffer/02-Gait_Track_and_Delete.png")
    plt.savefig(output_image_path)
    plt.close()

    # 可视化窗口3：未删减的步态周期插值曲线
    plt.figure(figsize=(15, 8))
    plt.suptitle(f"{data_name}:Origin Cycles", fontsize=16, fontname='Microsoft YaHei')
    plt.subplot(2, 4, 1)
    for start, end in gait_cycles_left:
        plt.plot(filtered_roll_thigh_left[start:end], color="skyblue", alpha=0.5)
    plt.title("Left Thigh Roll")

    plt.subplot(2, 4, 2)
    for start, end in gait_cycles_left:
        plt.plot(filtered_pitch_thigh_left[start:end], color="orange", alpha=0.5)
    plt.title("Left Thigh Pitch")

    plt.subplot(2, 4, 3)
    for start, end in gait_cycles_left:
        plt.plot(filtered_roll_shank_left[start:end], color="lightgreen", alpha=0.5)
    plt.title("Left Shank Roll")

    plt.subplot(2, 4, 4)
    for start, end in gait_cycles_left:
        plt.plot(filtered_pitch_shank_left[start:end], color="red", alpha=0.5)
    plt.title("Left Shank Pitch")

    plt.subplot(2, 4, 5)
    for start, end in gait_cycles_right:
        plt.plot(filtered_roll_thigh_right[start:end], color="skyblue", alpha=0.5)
    plt.title("Right Thigh Roll")

    plt.subplot(2, 4, 6)
    for start, end in gait_cycles_right:
        plt.plot(filtered_pitch_thigh_right[start:end], color="orange", alpha=0.5)
    plt.title("Right Thigh Pitch")

    plt.subplot(2, 4, 7)
    for start, end in gait_cycles_right:
        plt.plot(filtered_roll_shank_right[start:end], color="lightgreen", alpha=0.5)
    plt.title("Right Shank Roll")

    plt.subplot(2, 4, 8)
    for start, end in gait_cycles_right:
        plt.plot(filtered_pitch_shank_right[start:end], color="red", alpha=0.5)
    plt.title("Right Shank Pitch")
    plt.tight_layout()
    output_image_path = os.path.join(output_subfolder, f"buffer/03-Origin_Cycles.png")
    plt.savefig(output_image_path)
    plt.close()

    # 可视化窗口4：删减后的步态周期插值曲线
    interpolated_roll_thigh_left = interpolate_cycles(filtered_roll_thigh_left, normal_gait_cycles_left)
    interpolated_roll_shank_left = interpolate_cycles(filtered_roll_shank_left, normal_gait_cycles_left)
    interpolated_roll_thigh_right = interpolate_cycles(filtered_roll_thigh_right, normal_gait_cycles_right)
    interpolated_roll_shank_right = interpolate_cycles(filtered_roll_shank_right, normal_gait_cycles_right)
    interpolated_pitch_thigh_left = interpolate_cycles(filtered_pitch_thigh_left, normal_gait_cycles_left)
    interpolated_pitch_shank_left = interpolate_cycles(filtered_pitch_shank_left, normal_gait_cycles_left)
    interpolated_pitch_thigh_right = interpolate_cycles(filtered_pitch_thigh_right, normal_gait_cycles_right)
    interpolated_pitch_shank_right = interpolate_cycles(filtered_pitch_shank_right, normal_gait_cycles_right)

    plt.figure(figsize=(15, 8))
    plt.suptitle(f"{data_name}:Clean Cycles", fontsize=16, fontname='Microsoft YaHei')
    plt.subplot(2, 4, 1)
    for cycle in interpolated_roll_thigh_left:
        plt.plot(cycle, color="skyblue", alpha=0.5)
    plt.title("Left Thigh Roll (Clean)")

    plt.subplot(2, 4, 2)
    for cycle in interpolated_pitch_thigh_left:
        plt.plot(cycle, color="orange", alpha=0.5)
    plt.title("Left Thigh Pitch (Clean)")

    plt.subplot(2, 4, 3)
    for cycle in interpolated_roll_shank_left:
        plt.plot(cycle, color="lightgreen", alpha=0.5)
    plt.title("Left Shank Roll (Clean)")

    plt.subplot(2, 4, 4)
    for cycle in interpolated_pitch_shank_left:
        plt.plot(cycle, color="red", alpha=0.5)
    plt.title("Left Shank Pitch (Clean)")

    plt.subplot(2, 4, 5)
    for cycle in interpolated_roll_thigh_right:
        plt.plot(cycle, color="skyblue", alpha=0.5)
    plt.title("Right Thigh Roll (Clean)")

    plt.subplot(2, 4, 6)
    for cycle in interpolated_pitch_thigh_right:
        plt.plot(cycle, color="orange", alpha=0.5)
    plt.title("Right Thigh Pitch (Clean)")

    plt.subplot(2, 4, 7)
    for cycle in interpolated_roll_shank_right:
        plt.plot(cycle, color="lightgreen", alpha=0.5)
    plt.title("Right Shank Roll (Clean)")

    plt.subplot(2, 4, 8)
    for cycle in interpolated_pitch_shank_right:
        plt.plot(cycle, color="red", alpha=0.5)
    plt.title("Right Shank Pitch (Clean)")
    plt.tight_layout()
    output_image_path = os.path.join(output_subfolder, "buffer/04-Clean Cycles.png")
    plt.savefig(output_image_path)
    plt.close()

    # 可视化窗口5：Roll误差带图
    plt.figure(figsize=(15, 8))
    plt.suptitle(f"{data_name}:Roll", fontsize=16, fontname='Microsoft YaHei')
    plt.subplot(2, 2, 1)
    average_roll_thigh_left = average_cycles(interpolated_roll_thigh_left)
    max_roll_thigh_left = np.max(interpolated_roll_thigh_left, axis=0)
    min_roll_thigh_left = np.min(interpolated_roll_thigh_left, axis=0)
    plt.plot(average_roll_thigh_left, linestyle="--", color="skyblue", label="Average")
    plt.fill_between(range(len(max_roll_thigh_left)), min_roll_thigh_left, max_roll_thigh_left, color="skyblue", alpha=0.2)
    plt.title("Left Thigh")

    plt.subplot(2, 2, 2)
    average_roll_shank_left = average_cycles(interpolated_roll_shank_left)
    max_roll_shank_left = np.max(interpolated_roll_shank_left, axis=0)
    min_roll_shank_left = np.min(interpolated_roll_shank_left, axis=0)
    plt.plot(average_roll_shank_left, linestyle="--", color="lightgreen", label="Average")
    plt.fill_between(range(len(max_roll_shank_left)), min_roll_shank_left, max_roll_shank_left, color="lightgreen", alpha=0.2)
    plt.title("Left Shank")

    plt.subplot(2, 2, 3)
    average_roll_thigh_right = average_cycles(interpolated_roll_thigh_right)
    max_roll_thigh_right = np.max(interpolated_roll_thigh_right, axis=0)
    min_roll_thigh_right = np.min(interpolated_roll_thigh_right, axis=0)
    plt.plot(average_roll_thigh_right, linestyle="--", color="skyblue", label="Average")
    plt.fill_between(range(len(max_roll_thigh_right)), min_roll_thigh_right, max_roll_thigh_right, color="skyblue", alpha=0.2)
    plt.title("Right Thigh")

    plt.subplot(2, 2, 4)
    average_roll_shank_right = average_cycles(interpolated_roll_shank_right)
    max_roll_shank_right = np.max(interpolated_roll_shank_right, axis=0)
    min_roll_shank_right = np.min(interpolated_roll_shank_right, axis=0)
    plt.plot(average_roll_shank_right, linestyle="--", color="lightgreen", label="Average")
    plt.fill_between(range(len(max_roll_shank_right)), min_roll_shank_right, max_roll_shank_right, color="lightgreen", alpha=0.2)
    plt.title("Right Shank")
    plt.tight_layout()
    output_image_path = os.path.join(output_subfolder, f"{data_name}_Roll.png")
    plt.savefig(output_image_path)
    plt.close()

    # 可视化窗口6：Pitch误差带图
    plt.figure(figsize=(15, 8))
    plt.suptitle(f"{data_name}:Pitch", fontsize=16, fontname='Microsoft YaHei')
    plt.subplot(2, 2, 1)
    average_pitch_thigh_left = average_cycles(interpolated_pitch_thigh_left)
    max_pitch_thigh_left = np.max(interpolated_pitch_thigh_left, axis=0)
    min_pitch_thigh_left = np.min(interpolated_pitch_thigh_left, axis=0)
    plt.plot(average_pitch_thigh_left, linestyle="--", color="orange", label="Average")
    plt.fill_between(range(len(max_pitch_thigh_left)), min_pitch_thigh_left, max_pitch_thigh_left, color="orange", alpha=0.2)
    plt.title("Left Thigh")

    plt.subplot(2, 2, 2)
    average_pitch_shank_left = average_cycles(interpolated_pitch_shank_left)
    max_pitch_shank_left = np.max(interpolated_pitch_shank_left, axis=0)
    min_pitch_shank_left = np.min(interpolated_pitch_shank_left, axis=0)
    plt.plot(average_pitch_shank_left, linestyle="--", color="red", label="Average")
    plt.fill_between(range(len(max_pitch_shank_left)), min_pitch_shank_left, max_pitch_shank_left, color="red", alpha=0.2)
    plt.title("Left Shank")

    plt.subplot(2, 2, 3)
    average_pitch_thigh_right = average_cycles(interpolated_pitch_thigh_right)
    max_pitch_thigh_right = np.max(interpolated_pitch_thigh_right, axis=0)
    min_pitch_thigh_right = np.min(interpolated_pitch_thigh_right, axis=0)
    plt.plot(average_pitch_thigh_right, linestyle="--", color="orange", label="Average")
    plt.fill_between(range(len(max_pitch_thigh_right)), min_pitch_thigh_right, max_pitch_thigh_right, color="orange", alpha=0.2)
    plt.title("Right Thigh")

    plt.subplot(2, 2, 4)
    average_pitch_shank_right = average_cycles(interpolated_pitch_shank_right)
    max_pitch_shank_right = np.max(interpolated_pitch_shank_right, axis=0)
    min_pitch_shank_right = np.min(interpolated_pitch_shank_right, axis=0)
    plt.plot(average_pitch_shank_right, linestyle="--", color="red", label="Average")
    plt.fill_between(range(len(max_pitch_shank_right)), min_pitch_shank_right, max_pitch_shank_right, color="red", alpha=0.2)
    plt.title("Right Shank")
    plt.tight_layout()

    # 保存图片到对应的文件夹
    output_image_path = os.path.join(output_subfolder, f"{data_name}_Pitch.png")
    plt.savefig(output_image_path)
    plt.close()
    # plt.show()

for i in range(len(dataframes)):
    save_for_someone(dataframes[i], file_names[i])