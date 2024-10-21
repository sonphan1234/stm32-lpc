delete(instrfind); % Xóa tất cả các đối tượng kết nối cổng COM
s = serial('COM3', 'BaudRate', 9600);
fopen(s);

% Đọc dữ liệu từ cổng COM
for i = 1:11
    data = fgetl(s);
    disp(data);
end

fclose(s); % Đóng cổng COM
delete(s); % Xóa đối tượng kết nối cổng COM
clear s; % Xóa biến 's' khỏi workspace
