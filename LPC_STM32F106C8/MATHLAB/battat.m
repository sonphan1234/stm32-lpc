% Thiết lập kết nối UART
s = serial('COM3', 'BaudRate', 9600); % Thay COM3 bằng cổng COM mà thiết bị của bạn đang sử dụng
fopen(s); % Mở cổng UART

% Đọc dữ liệu từ UART
data = fscanf(s, '%s');

% Đóng cổng UART
fclose(s);
delete(s);

% Phân tích dữ liệu
lines = strsplit(data, '\n');
adc_values = [];
lpc_values = [];

for i = 1:length(lines)
    line = lines{i};
    if contains(line, 'ADC Value:')
        adc_value = sscanf(line, 'ADC Value: %d');
        adc_values = [adc_values; adc_value];
    elseif contains(line, 'LPC')
        lpc_value = sscanf(line, 'LPC[%*d]: %f');
        lpc_values = [lpc_values; lpc_value'];
    end
end

% Tái tạo tín hiệu từ các hệ số LPC
reconstructed_data = [];
for i = 1:length(adc_values)
    lpc_coeffs = lpc_values((i-1)*10 + 1:i*10)';
    reconstructed_signal = filter([0 -lpc_coeffs(2:end)], 1, adc_values(i));
    reconstructed_data = [reconstructed_data; reconstructed_signal];
end

% Vẽ biểu đồ dữ liệu ADC và dữ liệu tái tạo từ LPC
figure;
subplot(2, 1, 1);
plot(adc_values);
title('Biểu đồ dữ liệu ADC');
xlabel('Thời gian');
ylabel('Amplitude');

subplot(2, 1, 2);
plot(reconstructed_data);
title('Biểu đồ dữ liệu tái tạo từ LPC');
xlabel('Thời gian');
ylabel('Amplitude');

% Phát lại âm thanh tái tạo
soundsc(reconstructed_data, 8000); % Giả sử tần số mẫu là 8000 Hz
