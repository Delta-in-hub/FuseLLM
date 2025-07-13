import os

def main():
    # 配置参数
    start_dir = '.'  # 起始目录（当前目录）
    output_file = 'all_files_output.txt'  # 输出文件名
    exclude_dirs = ['.git','build','.github','.cache','assets','external','doxygen']  # 要排除的目录列表
    include_suffixes = {
        '.py', '.cpp', '.c', '.h', '.hpp', '.txt', '.md', '.sh',
        '.cmake', '.json', '.xml', '.yml', '.yaml', '.ini', '.log',
        '.sql', '.html', '.css', '.js', '.ts', 'readme', 'license'
    }

    # 获取绝对路径
    start_dir = os.path.abspath(start_dir)
    output_abs = os.path.abspath(output_file)

    # 打开输出文件
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for root, dirs, files in os.walk(start_dir):
            # 排除指定的目录
            dirs[:] = [d for d in dirs if d not in exclude_dirs]

            for file in files:
                file_path = os.path.join(root, file)
                file_abs = os.path.abspath(file_path)

                # 跳过输出文件本身
                if file_abs == output_abs:
                    continue

                # 获取相对路径
                rel_path = os.path.relpath(file_path, start_dir)

                # 检查文件后缀
                suffix = os.path.splitext(file)[1].lower()
                if include_suffixes and suffix not in include_suffixes:
                    continue

                # 读取文件内容
                try:
                    with open(file_path, 'r', encoding='utf-8') as f:
                        content = f.read()
                except UnicodeDecodeError:
                    outfile.write(f"无法读取文件 {rel_path}：文件可能不是 UTF-8 编码，或为二进制文件。\n\n")
                    continue
                except Exception as e:
                    outfile.write(f"读取文件 {rel_path} 时出错：{str(e)}\n\n")
                    continue

                # 写入文件信息到输出文件
                outfile.write('-' * 50 + '\n')
                outfile.write(f'File: {rel_path}\n')
                outfile.write('-' * 50 + '\n')
                outfile.write(content)
                outfile.write('\n\n')  # 文件之间空两行

    print(f"文件内容已成功保存到 {output_file}")

if __name__ == '__main__':
    main()