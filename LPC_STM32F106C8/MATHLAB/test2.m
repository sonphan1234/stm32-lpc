% Tạo đối tượng serialport
s = serialport('COM3', 9600); % Thay 'COMx' bằng cổng serial của bạn
configureTerminator(s, "LF");

% Đọc dữ liệu từ cổng serial và lưu vào biến data
data = read(s, s.NumBytesAvailable, "char");

% Chia dữ liệu thành các phần riêng biệt cho ADC value và LPC coefficients
data_str = split(data, "\n");

% Lấy ADC value từ dữ liệu
adc_values = [];
for i = 1:length(data_str)
    if startsWith(data_str(i), "ADC Value:")
        adc_values(end+1) = str2double(extractAfter(data_str(i), "ADC Value: "));
    end
end

% Lấy LPC coefficients từ dữ liệu
lpc_coefficients = zeros(length(data_str), 10); % Khởi tạo mảng lpc_coefficients
for i = 1:length(data_str)
    if startsWith(data_str(i), "LPC[")
        temp = str2double(split(extractAfter(data_str(i), "LPC["), ": "));
        lpc_coefficients(i, :) = temp;
    end
end

% Vẽ biểu đồ ADC value
subplot(2,1,1);
plot(1:length(adc_values), adc_values);
title('ADC Value');

% Vẽ biểu đồ LPC coefficients
subplot(2,1,2);
plot(lpc_coefficients);
title('LPC Coefficients');

% Đóng cổng serial
delete(s);
