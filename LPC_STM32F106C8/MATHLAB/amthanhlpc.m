% Mở cổng UART
s = serial('COM3', 'BaudRate', 115200); % Thay COM3 bằng cổng COM thực tế của bạn
fopen(s); % Mở cổng UART

% Đọc dữ liệu từ UART
data_buffer = ''; % Tạo buffer rỗng để lưu dữ liệu
timeout = 10; % Thời gian chờ tối đa là 10 giây
start_time = tic;

while length(data_buffer) < 100 * 2 % Đọc 100 mẫu từ STM32F10x
    if toc(start_time) > timeout
        error('Timeout: Không đủ dữ liệu mẫu từ STM32F10x');
    end
    data_buffer = [data_buffer, fscanf(s)]; % Đọc dữ liệu từ UART và nối vào buffer
end

fclose(s); % Đóng cổng UART

% Tách dữ liệu thành các phần tử riêng biệt
data_lines = strsplit(data_buffer, '\n');

% Khởi tạo mảng lưu trữ dữ liệu
adc_values = [];
lpc_coefficients = [];

% Xử lý từng dòng dữ liệu
for i = 1:length(data_lines)
    line = data_lines{i};
    if ~isempty(line)
        if mod(i, 2) == 1
            % Đọc giá trị ADC từ dòng lẻ
            adc_value = sscanf(line, '%d');
            adc_values = [adc_values; adc_value];
        else
            % Đọc các hệ số LPC từ dòng chẵn
            lpc_coeffs = sscanf(line, '%f');
            lpc_coefficients = [lpc_coefficients; lpc_coeffs'];
        end
    end
end

% Chỉ lấy 100 mẫu đầu tiên
adc_values = adc_values(1:100);
lpc_coefficients = lpc_coefficients(1:100, :);

% Tái tạo dữ liệu âm thanh từ các hệ số LPC và giá trị ADC
reconstructed_audio = zeros(100, 1);
for i = 1:100
    % Tái tạo dữ liệu âm thanh từ các hệ số LPC và giá trị ADC
    reconstructed_data = zeros(100, 1);
    Reconstruct_ADC_Data(lpc_coefficients(i,:), 100, adc_values(i), reconstructed_data);
    reconstructed_audio = reconstructed_audio + reconstructed_data;
end

% Tần số lấy mẫu và âm thanh tái tạo
Fs = 8000;
sound(reconstructed_audio, Fs);

% Vẽ biểu đồ tín hiệu ADC gốc
figure;
subplot(2, 1, 1);
plot(adc_values);
title('Tín hiệu ADC gốc');
xlabel('Mẫu');
ylabel('Giá trị ADC');

% Vẽ biểu đồ hệ số LPC
subplot(2, 1, 2);
plot(lpc_coefficients);
title('Các hệ số LPC');
xlabel('Bậc');
ylabel('Giá trị');

% Lưu tín hiệu tái tạo vào một tệp âm thanh để nghe lại
audiowrite('reconstructed_audio.wav', reconstructed_audio, Fs);
