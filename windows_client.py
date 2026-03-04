import socket
import struct
import time
import threading
import json
import numpy as np
from datetime import datetime
import sys
import os

class IQClient:
    """
    IQ数据客户端类，用于连接ARM服务器并发送/接收数据
    """
    
    def __init__(self, server_ip, server_port):
        """
        初始化客户端
        
        Args:
            server_ip: 服务器IP地址
            server_port: 服务器端口
        """
        self.server_ip = server_ip
        self.server_port = server_port
        self.sock = None
        self.connected = False
        self.sequence_num = 1
        
        # 协议常量
        self.PACKET_SIZE = 4096
        self.HEADER_SIZE = 8
        self.TRAILER_SIZE = 2
        
        # 包类型定义
        self.PACKET_TYPE_COMMAND = 0x01
        self.PACKET_TYPE_IQ_DATA = 0x02
        self.PACKET_TYPE_ACK = 0x03
        self.PACKET_TYPE_STATS = 0x04
        self.PACKET_TYPE_CONTROL = 0x05
        
        # 命令类型定义
        self.CMD_START_STREAM = 0x01
        self.CMD_STOP_STREAM = 0x02
        self.CMD_START_RECORD = 0x03
        self.CMD_STOP_RECORD = 0x04
        self.CMD_PERFORMANCE_TEST = 0x05
        self.CMD_GET_STATS = 0x06
        self.CMD_RESET_STATS = 0x07
        self.CMD_SET_PARAMS = 0x08
        
    def connect(self, timeout=5):
        """
        连接到服务器
        
        Args:
            timeout: 连接超时时间(秒)
            
        Returns:
            bool: 连接是否成功
        """
        try:
            print(f"正在连接服务器 {self.server_ip}:{self.server_port}...")
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(timeout)
            self.sock.connect((self.server_ip, self.server_port))
            
            # 设置socket选项
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            
            self.connected = True
            print("连接成功!")
            return True
            
        except socket.timeout:
            print("连接超时")
            return False
        except ConnectionRefusedError:
            print("连接被拒绝，请检查服务器是否运行")
            return False
        except Exception as e:
            print(f"连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.sock:
            self.sock.close()
            self.sock = None
        self.connected = False
        print("已断开连接")
    
    def calculate_crc16(self, data):
        """
        计算CRC16校验值
        
        Args:
            data: 字节数据
            
        Returns:
            int: CRC16值
        """
        crc = 0xFFFF
        polynomial = 0x1021
        
        for byte in data:
            crc ^= (byte << 8)
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ polynomial
                else:
                    crc <<= 1
                crc &= 0xFFFF
        
        return crc
    
    def create_packet(self, packet_type, payload, sequence_num=None):
        """
        创建数据包
        
        Args:
            packet_type: 包类型
            payload: 有效载荷(字节)
            sequence_num: 序列号(如果为None则使用自动递增)
            
        Returns:
            bytes: 完整的数据包
        """
        if sequence_num is None:
            sequence_num = self.sequence_num
            self.sequence_num += 1
        
        # 初始化数据包缓冲区
        packet = bytearray(self.PACKET_SIZE)
        
        # 包类型
        packet[0] = packet_type
        
        # 序列号(网络字节序)
        struct.pack_into('!I', packet, 1, sequence_num)
        
        # 有效长度(网络字节序)
        valid_length = len(payload)
        struct.pack_into('!I', packet, 5, valid_length)
        
        # 时间戳(网络字节序)
        timestamp = int(time.time() * 1000)  # 毫秒时间戳
        struct.pack_into('!Q', packet, 9, timestamp)
        
        # 填充有效载荷
        if valid_length > 0:
            start_pos = self.HEADER_SIZE + 8  # 1+4+4+8=17字节头部
            end_pos = start_pos + min(valid_length, self.PACKET_SIZE - start_pos - self.TRAILER_SIZE)
            packet[start_pos:end_pos] = payload[:end_pos-start_pos]
        
        # 计算CRC(对整个数据包除CRC部分)
        crc_data = packet[:self.PACKET_SIZE - self.TRAILER_SIZE]
        crc = self.calculate_crc16(crc_data)
        
        # 填充CRC(网络字节序)
        struct.pack_into('!H', packet, self.PACKET_SIZE - self.TRAILER_SIZE, crc)
        
        return bytes(packet)
    
    def parse_packet(self, packet_data):
        """
        解析接收到的数据包
        
        Args:
            packet_data: 接收到的数据包字节
            
        Returns:
            dict: 解析后的数据包信息
        """
        if len(packet_data) < self.PACKET_SIZE:
            return None
        
        try:
            # 包类型
            packet_type = packet_data[0]
            
            # 序列号
            sequence_num = struct.unpack_from('!I', packet_data, 1)[0]
            
            # 有效长度
            valid_length = struct.unpack_from('!I', packet_data, 5)[0]
            
            # 时间戳
            timestamp = struct.unpack_from('!Q', packet_data, 9)[0]
            
            # 提取有效载荷
            payload_start = self.HEADER_SIZE + 8
            payload_end = payload_start + valid_length
            payload = packet_data[payload_start:payload_end]
            
            # 提取CRC
            crc = struct.unpack_from('!H', packet_data, self.PACKET_SIZE - self.TRAILER_SIZE)[0]
            
            # 验证CRC
            crc_data = packet_data[:self.PACKET_SIZE - self.TRAILER_SIZE]
            calculated_crc = self.calculate_crc16(crc_data)
            crc_valid = (calculated_crc == crc)
            
            return {
                'packet_type': packet_type,
                'sequence_num': sequence_num,
                'valid_length': valid_length,
                'timestamp': timestamp,
                'payload': payload,
                'crc': crc,
                'crc_valid': crc_valid
            }
            
        except Exception as e:
            print(f"解析数据包失败: {e}")
            return None
    
    def send_packet(self, packet):
        """
        发送数据包
        
        Args:
            packet: 数据包字节
            
        Returns:
            bool: 发送是否成功
        """
        if not self.connected or not self.sock:
            print("未连接到服务器")
            return False
        
        try:
            total_sent = 0
            while total_sent < len(packet):
                sent = self.sock.send(packet[total_sent:])
                if sent == 0:
                    print("连接已关闭")
                    self.connected = False
                    return False
                total_sent += sent
            return True
            
        except Exception as e:
            print(f"发送数据包失败: {e}")
            self.connected = False
            return False
    
    def receive_packet(self, timeout=5):
        """
        接收数据包
        
        Args:
            timeout: 接收超时时间(秒)
            
        Returns:
            dict: 解析后的数据包信息，失败返回None
        """
        if not self.connected or not self.sock:
            print("未连接到服务器")
            return None
        
        try:
            self.sock.settimeout(timeout)
            packet_data = bytearray()
            
            # 接收完整数据包
            while len(packet_data) < self.PACKET_SIZE:
                chunk = self.sock.recv(self.PACKET_SIZE - len(packet_data))
                if not chunk:
                    print("连接已关闭")
                    self.connected = False
                    return None
                packet_data.extend(chunk)
            
            return self.parse_packet(bytes(packet_data))
            
        except socket.timeout:
            print("接收超时")
            return None
        except Exception as e:
            print(f"接收数据包失败: {e}")
            self.connected = False
            return None
    
    def send_command(self, choice, sweep_start_freq, read_length_type):
        """
        发送命令包
        
        Args:
            choice: 处理选项(1:检测, 2:识别通信信号, 3:识别干扰信号)
            sweep_start_freq: 起始频率(MHz)
            read_length_type: 读取长度类型(1:8M, 2:64M, 3:512M)
            
        Returns:
            bool: 命令发送是否成功
        """
        # 创建命令有效载荷
        payload = bytearray()
        payload.append(choice)  # 1字节choice
        payload.extend(struct.pack('!i', sweep_start_freq))  # 4字节频率
        payload.append(read_length_type)  # 1字节长度类型
        
        # 创建命令包
        packet = self.create_packet(self.PACKET_TYPE_COMMAND, payload)
        
        # 发送命令包
        if not self.send_packet(packet):
            return False
        
        print(f"已发送命令: choice={choice}, freq={sweep_start_freq}MHz, len_type={read_length_type}")
        
        # 等待确认
        response = self.receive_packet(timeout=2)
        if response and response['packet_type'] == self.PACKET_TYPE_ACK:
            print("收到服务器确认")
            return True
        else:
            print("未收到服务器确认")
            return False
    
    def send_iq_data(self, i_samples, q_samples=None):
        """
        发送IQ数据包
        
        Args:
            i_samples: I通道样本列表(整数)
            q_samples: Q通道样本列表(整数，如果为None则生成随机数据)
            
        Returns:
            bool: 发送是否成功
        """
        if q_samples is None:
            # 如果没有提供Q样本，生成随机数据
            q_samples = np.random.randint(-32768, 32767, len(i_samples)).tolist()
        
        if len(i_samples) != len(q_samples):
            print("I和Q样本数量不匹配")
            return False
        
        # 创建IQ数据有效载荷
        payload = bytearray()
        
        # 添加序列号(4字节)
        seq_num = self.sequence_num
        payload.extend(struct.pack('!I', seq_num))
        
        # 添加时间戳(8字节)
        timestamp = int(time.time() * 1000000)  # 微秒时间戳
        payload.extend(struct.pack('!Q', timestamp))
        
        # 添加样本数(4字节)
        sample_count = len(i_samples)
        payload.extend(struct.pack('!I', sample_count))
        
        # 添加IQ数据
        for i, q in zip(i_samples, q_samples):
            # 将整数转换为int16并打包(小端字节序)
            payload.extend(struct.pack('<h', int(i)))
            payload.extend(struct.pack('<h', int(q)))
        
        # 创建IQ数据包
        packet = self.create_packet(self.PACKET_TYPE_IQ_DATA, payload)
        
        # 发送数据包
        if not self.send_packet(packet):
            return False
        
        print(f"已发送IQ数据包: 序列号={seq_num}, 样本数={sample_count}")
        
        # 等待处理完成确认
        response = self.receive_packet(timeout=5)
        if response and response['packet_type'] == self.PACKET_TYPE_ACK:
            print("收到处理完成确认")
            return True
        else:
            print("未收到处理完成确认")
            return False
    
    def send_control_command(self, start_channel, end_channel):
        """
        发送控制命令包
        
        Args:
            start_channel: 起始子信道
            end_channel: 结束子信道
            
        Returns:
            bool: 发送是否成功
        """
        # 创建控制命令有效载荷
        payload = bytearray()
        payload.append(start_channel)  # 1字节起始信道
        payload.extend(struct.pack('!d', end_channel))  # 8字节结束信道
        
        # 创建控制包
        packet = self.create_packet(self.PACKET_TYPE_CONTROL, payload)
        
        # 发送控制包
        if not self.send_packet(packet):
            return False
        
        print(f"已发送控制命令: 起始信道={start_channel}, 结束信道={end_channel}")
        return True
    
    def performance_test(self, duration_seconds=10, sample_rate=1000000):
        """
        性能测试
        
        Args:
            duration_seconds: 测试持续时间(秒)
            sample_rate: 采样率(Hz)
            
        Returns:
            dict: 测试结果
        """
        print(f"开始性能测试，持续时间: {duration_seconds}秒")
        
        start_time = time.time()
        packets_sent = 0
        samples_sent = 0
        
        # 计算每包样本数(假设8字节每样本)
        samples_per_packet = 500  # 500样本每包
        
        while time.time() - start_time < duration_seconds:
            # 生成测试IQ数据
            i_samples = np.random.randint(-32768, 32767, samples_per_packet).tolist()
            
            # 发送IQ数据
            if self.send_iq_data(i_samples):
                packets_sent += 1
                samples_sent += samples_per_packet
            else:
                print("发送失败，停止测试")
                break
            
            # 短暂休眠以避免发送过快
            time.sleep(0.01)
        
        elapsed_time = time.time() - start_time
        throughput = (samples_sent * 4) / elapsed_time / (1024 * 1024)  # MB/s
        
        result = {
            'duration': elapsed_time,
            'packets_sent': packets_sent,
            'samples_sent': samples_sent,
            'throughput_mbps': throughput,
            'packet_rate': packets_sent / elapsed_time
        }
        
        print(f"性能测试完成:")
        print(f"  持续时间: {elapsed_time:.2f}秒")
        print(f"  发送包数: {packets_sent}")
        print(f"  发送样本: {samples_sent}")
        print(f"  吞吐量: {throughput:.2f} MB/s")
        print(f"  包率: {packets_sent/elapsed_time:.2f} 包/秒")
        
        return result

def test_scenario_1(client):
    """
    测试场景1: 基本连接和命令测试
    """
    print("\n=== 测试场景1: 基本连接测试 ===")
    
    # 连接服务器
    if not client.connect():
        return False
    
    # 发送命令
    success = client.send_command(
        choice=1,  # 信号检测
        sweep_start_freq=1000,  # 1000MHz
        read_length_type=1  # 8M
    )
    
    if success:
        print("基本命令测试通过")
    else:
        print("基本命令测试失败")
    
    # 断开连接
    client.disconnect()
    return success

def test_scenario_2(client):
    """
    测试场景2: IQ数据发送测试
    """
    print("\n=== 测试场景2: IQ数据发送测试 ===")
    
    # 连接服务器
    if not client.connect():
        return False
    
    # 发送命令
    client.send_command(
        choice=1,  # 信号检测
        sweep_start_freq=2000,  # 2000MHz
        read_length_type=1  # 8M
    )
    
    # 生成测试IQ数据
    num_samples = 1000
    print(f"生成{num_samples}个IQ样本...")
    
    # 生成正弦波作为I数据
    t = np.linspace(0, 1, num_samples)
    i_samples = (np.sin(2 * np.pi * 10 * t) * 32767).astype(int).tolist()
    
    # 发送IQ数据
    success = client.send_iq_data(i_samples)
    
    if success:
        print("IQ数据发送测试通过")
    else:
        print("IQ数据发送测试失败")
    
    # 断开连接
    client.disconnect()
    return success

def test_scenario_3(client):
    """
    测试场景3: 性能测试
    """
    print("\n=== 测试场景3: 性能测试 ===")
    
    # 连接服务器
    if not client.connect():
        return False
    
    # 发送命令
    client.send_command(
        choice=1,  # 信号检测
        sweep_start_freq=3000,  # 3000MHz
        read_length_type=1  # 8M
    )
    
    # 运行性能测试
    result = client.performance_test(duration_seconds=5)
    
    # 断开连接
    client.disconnect()
    
    # 评估结果
    if result['throughput_mbps'] > 1.0:  # 要求吞吐量大于1MB/s
        print("性能测试通过")
        return True
    else:
        print("性能测试失败: 吞吐量过低")
        return False

def test_scenario_4(client):
    """
    测试场景4: 完整信号处理流程测试
    """
    print("\n=== 测试场景4: 完整信号处理流程测试 ===")
    
    # 连接服务器
    if not client.connect():
        return False
    
    # 测试不同处理选项
    test_cases = [
        (1, "信号检测", 2400),
        (2, "通信信号识别", 1800),
        (3, "干扰信号识别", 900),
    ]
    
    all_passed = True
    
    for choice, description, freq in test_cases:
        print(f"\n测试: {description}")
        
        # 发送命令
        if not client.send_command(choice, freq, 1):
            print(f"{description}命令发送失败")
            all_passed = False
            continue
        
        # 生成测试数据
        num_samples = 2000
        t = np.linspace(0, 1, num_samples)
        
        if choice == 1:  # 检测
            # 生成AM信号
            carrier_freq = 100
            i_samples = (np.sin(2 * np.pi * carrier_freq * t) * 
                        (1 + 0.5 * np.sin(2 * np.pi * 5 * t)) * 20000).astype(int).tolist()
        elif choice == 2:  # 通信信号
            # 生成QPSK信号
            bits = np.random.randint(0, 2, num_samples)
            i_samples = (np.cos(bits * np.pi/2) * 30000).astype(int).tolist()
        else:  # 干扰信号
            # 生成脉冲干扰
            i_samples = np.zeros(num_samples, dtype=int)
            for i in range(0, num_samples, 100):
                i_samples[i:i+20] = 25000
            i_samples = i_samples.tolist()
        
        # 发送IQ数据
        if client.send_iq_data(i_samples):
            print(f"{description}测试通过")
        else:
            print(f"{description}测试失败")
            all_passed = False
    
    # 断开连接
    client.disconnect()
    return all_passed

def interactive_mode(client):
    """
    交互式测试模式
    """
    print("\n=== 交互式测试模式 ===")
    print("输入命令:")
    print("  connect - 连接服务器")
    print("  disconnect - 断开连接")
    print("  command <choice> <freq> <len_type> - 发送命令")
    print("  send_iq <num_samples> - 发送IQ数据")
    print("  control <start> <end> - 发送控制命令")
    print("  perf_test <duration> - 性能测试")
    print("  quit - 退出")
    
    while True:
        try:
            cmd_input = input("\n> ").strip().lower()
            if not cmd_input:
                continue
            
            parts = cmd_input.split()
            cmd = parts[0]
            
            if cmd == "quit":
                print("退出交互模式")
                break
                
            elif cmd == "connect":
                if client.connect():
                    print("连接成功")
                else:
                    print("连接失败")
                    
            elif cmd == "disconnect":
                client.disconnect()
                
            elif cmd == "command" and len(parts) == 4:
                try:
                    choice = int(parts[1])
                    freq = int(parts[2])
                    len_type = int(parts[3])
                    
                    if client.send_command(choice, freq, len_type):
                        print("命令发送成功")
                    else:
                        print("命令发送失败")
                        
                except ValueError:
                    print("参数错误: 请输入整数")
                    
            elif cmd == "send_iq" and len(parts) == 2:
                try:
                    num_samples = int(parts[1])
                    
                    # 生成随机IQ数据
                    i_samples = np.random.randint(-32768, 32767, num_samples).tolist()
                    
                    if client.send_iq_data(i_samples):
                        print(f"成功发送{num_samples}个IQ样本")
                    else:
                        print("发送失败")
                        
                except ValueError:
                    print("参数错误: 请输入整数")
                    
            elif cmd == "control" and len(parts) == 3:
                try:
                    start_ch = int(parts[1])
                    end_ch = float(parts[2])
                    
                    if client.send_control_command(start_ch, end_ch):
                        print("控制命令发送成功")
                    else:
                        print("控制命令发送失败")
                        
                except ValueError:
                    print("参数错误")
                    
            elif cmd == "perf_test" and len(parts) == 2:
                try:
                    duration = float(parts[1])
                    client.performance_test(duration_seconds=duration)
                except ValueError:
                    print("参数错误: 请输入数字")
                    
            else:
                print("未知命令或参数错误")
                
        except KeyboardInterrupt:
            print("\n用户中断")
            break
        except Exception as e:
            print(f"错误: {e}")

def main():
    """主函数"""
    print("=" * 50)
    print("Windows端香橙派ARM服务器测试程序")
    print("=" * 50)
    
    # 服务器配置
    SERVER_IP = "192.168.1.170"  # 根据实际情况修改
    SERVER_PORT = 8888
    
    # 创建客户端实例
    client = IQClient(SERVER_IP, SERVER_PORT)
    
    # 检查命令行参数
    if len(sys.argv) > 1:
        mode = sys.argv[1].lower()
        
        if mode == "auto":
            # 自动运行所有测试场景
            print("运行自动化测试...")
            
            tests = [
                ("基本连接测试", test_scenario_1),
                ("IQ数据发送测试", test_scenario_2),
                ("性能测试", test_scenario_3),
                ("完整流程测试", test_scenario_4),
            ]
            
            passed = 0
            total = len(tests)
            
            for test_name, test_func in tests:
                try:
                    if test_func(client):
                        print(f"✓ {test_name}: 通过")
                        passed += 1
                    else:
                        print(f"✗ {test_name}: 失败")
                except Exception as e:
                    print(f"✗ {test_name}: 异常 - {e}")
            
            print(f"\n测试完成: {passed}/{total} 通过")
            
        elif mode == "interactive":
            # 交互式模式
            interactive_mode(client)
            
        elif mode == "single":
            # 运行单个测试场景
            if len(sys.argv) > 2:
                scenario_num = int(sys.argv[2])
                if scenario_num == 1:
                    test_scenario_1(client)
                elif scenario_num == 2:
                    test_scenario_2(client)
                elif scenario_num == 3:
                    test_scenario_3(client)
                elif scenario_num == 4:
                    test_scenario_4(client)
                else:
                    print(f"未知测试场景: {scenario_num}")
            else:
                print("请指定测试场景编号 (1-4)")
        
        else:
            print(f"未知模式: {mode}")
            print("可用模式: auto, interactive, single")
            
    else:
        # 默认运行完整测试
        print("未指定模式，运行完整自动化测试...")
        
        tests = [
            ("基本连接测试", test_scenario_1),
            ("IQ数据发送测试", test_scenario_2),
            ("性能测试", test_scenario_3),
            ("完整流程测试", test_scenario_4),
        ]
        
        passed = 0
        total = len(tests)
        
        for test_name, test_func in tests:
            try:
                if test_func(client):
                    print(f"✓ {test_name}: 通过")
                    passed += 1
                else:
                    print(f"✗ {test_name}: 失败")
            except Exception as e:
                print(f"✗ {test_name}: 异常 - {e}")
        
        print(f"\n测试完成: {passed}/{total} 通过")
        
        # 询问是否进入交互模式
        try:
            response = input("\n是否进入交互模式? (y/n): ").strip().lower()
            if response == 'y':
                interactive_mode(client)
        except:
            pass

if __name__ == "__main__":
    main()