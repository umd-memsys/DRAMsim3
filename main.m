close all
clear
clc

data = csvread('./build/ddr3-output-final_power_temperature.csv', 1, 0);
pos = csvread('./build/ddr3-output-bank_position.csv', 1, 0); 

X = max(data(:,2));
Y = max(data(:,3));
Z = max(data(:,4));
fprintf('Floorplan: (%d, %d, %d)\n', X+1, Y+1, Z+1);

power = zeros(X+1, Y+1, Z+1);
temperature = power; 

for i = 1 : size(data, 1)
    power(data(i,2)+1, data(i,3)+1, data(i,4)+1) = data(i,5);
    temperature(data(i,2)+1, data(i,3)+1, data(i,4)+1) = data(i,6);
end

figP = figure;
% surf(power(:,:,1));
% view(0, -90);
% shading interp
imagesc(power(:,:,1));
title('Power');
hold on 

figT = figure;
imagesc(temperature(:,:,1));
title('Temperature');
hold on 

figure(figP); 
for i = 1 : size(pos, 1)
    x_ = pos(i,2); 
    y_ = pos(i,4);
    w_ = pos(i,3) - pos(i,2) + 1; 
    l_ = pos(i,5) - pos(i,4) + 1; 
    rectangle('Position', [y_+0.5, x_+0.5, l_, w_], 'EdgeColor', 'r'); 
    text(y_+0.5+l_/4, x_+0.5+w_/2, ['B' num2str(i-1)], 'Color', 'r', 'FontWeight', 'bold'); 
end
colorbar
xlabel('Y (Colomn)');
ylabel('X (Row)');

figure(figT); 
for i = 1 : size(pos, 1)
    x_ = pos(i,2); 
    y_ = pos(i,4);
    w_ = pos(i,3) - pos(i,2) + 1; 
    l_ = pos(i,5) - pos(i,4) + 1; 
    rectangle('Position', [y_+0.5, x_+0.5, l_, w_], 'EdgeColor', 'r'); 
    text(y_+0.5+l_/4, x_+0.5+w_/2, ['B' num2str(i-1)], 'Color', 'r', 'FontWeight', 'bold'); 
end
colorbar
xlabel('Y (Colomn)');
ylabel('X (Row)');

