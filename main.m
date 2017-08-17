close all
clear
clc

mem = 'ddr4'; 

data = csvread(['./build/' mem '-output-final_power_temperature.csv'], 1, 0);
pos = csvread(['./build/' mem '-output-bank_position.csv'], 1, 0); 

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

layer_t = 1; 

figP = figure;
% surf(power(:,:,1));
% view(0, -90);
% shading interp
imagesc(power(:,:,layer_t));
title('Power');
hold on 

figT = figure;
imagesc(temperature(:,:,layer_t));
title('Temperature');
hold on 

figure(figP); 
for i = 1 : size(pos, 1)
    if pos(i, 7) + 1 == layer_t
        x_ = pos(i,3); 
        y_ = pos(i,5);
        w_ = pos(i,4) - pos(i,3) + 1; 
        l_ = pos(i,6) - pos(i,5) + 1; 
        if l_ >= w_
            rot = 0;
        else
            rot = 90;
        end
        rectangle('Position', [y_+0.5, x_+0.5, l_, w_], 'EdgeColor', 'r'); 
        text(y_+0.5+l_/4, x_+0.5+w_/2, ['R' num2str(pos(i,1)) 'B' num2str(pos(i,2))], 'Color', 'r', 'FontWeight', 'bold', 'Rotation', rot); 
    end
end
colorbar
xlabel('Y (Colomn)');
ylabel('X (Row)');

figure(figT); 
for i = 1 : size(pos, 1)
    if pos(i, 7) + 1 == layer_t
        x_ = pos(i,3); 
        y_ = pos(i,5);
        w_ = pos(i,4) - pos(i,3) + 1; 
        l_ = pos(i,6) - pos(i,5) + 1; 
        rectangle('Position', [y_+0.5, x_+0.5, l_, w_], 'EdgeColor', 'r'); 
        if l_ >= w_
            rot = 0;
        else
            rot = 90;
        end
        text(y_+0.5+l_/4, x_+0.5+w_/2, ['R' num2str(pos(i,1)) 'B' num2str(pos(i,2))], 'Color', 'r', 'FontWeight', 'bold', 'Rotation', rot); 
    end
end
colorbar
xlabel('Y (Colomn)');
ylabel('X (Row)');