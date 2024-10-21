% Kiểm tra thiết bị âm thanh
devices = audiodevinfo;

% Hiển thị thông tin về các thiết bị âm thanh
disp('Available Input Devices:');
disp(devices.input);

disp('Available Output Devices:');
disp(devices.output);

% Thiết lập thông số thu âm
fs = 44100; % Tần số lấy mẫu (Hz)
nBits = 16; % Kích thước mẫu (bit)
nChannels = 1; % Số kênh (1 cho mono, 2 cho stereo)
recObj = audiorecorder(fs, nBits, nChannels);

% Bắt đầu thu âm
disp('Start speaking.');
recordblocking(recObj, 5); % Ghi âm trong 5 giây
disp('End of Recording.');

% Phát lại âm thanh vừa ghi
play(recObj);

% Lấy dữ liệu âm thanh
audioData = getaudiodata(recObj);

% Hiển thị đồ thị của tín hiệu âm thanh
figure;
plot(audioData);
title('Recorded Audio Signal');
xlabel('Sample Number');
ylabel('Amplitude');

% Lưu âm thanh vào file
audiowrite('myRecording.wav', audioData, fs);

disp('Audio recording saved to "myRecording.wav".');
