function g = DiscreteGaussian(sigma, tau, fraction)
%
% g = DiscreteGaussian(sigma, tau, fraction)
%
%  Creates a discrete Gaussian kernel with specified sigma.  The "timestep"
%  for approximation can be specified in 'tau' (tau <= 0.25 for stability).
%  Default is tau = 0.1.  Better accuracy (but larger kernel size) is
%  achieved as tau approaches zero.  You can also specify the fraction of
%  the area under the curve to keep (default is .9999) to reduce the kernel
%  size by throwing out near-zero elements.  Note that the returned kernel
%  will be forced to sum to one.
%
% -------------
% Andrew Stein
%


if nargin < 2 || isempty(tau)
    %timestep
    tau = 0.1;
end

if nargin < 3 || isempty(fraction)
	fraction = .9999;
end

% total number of iterations
K_total = (sigma^2) / (2*tau);

% Divide up iterations into integer, tenthts, hundredths...
decimal = [1 .1 .01];
K = floor(K_total);
for i_dec = 2:length(decimal)
	K(i_dec) = round( (K_total - sum(K(1:(i_dec-1))))/decimal(i_dec) );
end

% Progressively convolve for the correct number of iterations, including
% any additional iterations at reduced timestep to account for fraction
% numbers of iterations according to the specified sigma:
g = 1;
for i_dec = 1:length(decimal)
	crnt_tau = tau*decimal(i_dec);
	g_update = [crnt_tau 1-2*crnt_tau crnt_tau];

	for k = 1:K(i_dec)
		g = conv(g, g_update);
	end
end

% Keep the specified fraction of the area under the curve:
mid_pt = (length(g)+1)/2;
half_area = cumsum(g(1:mid_pt));
keep = half_area >= (1-fraction)/2;
keep = [keep fliplr(keep(1:end-1))];
g = g(keep);

% Re-normalize to ensure the kernel sums to one:
g = g/sum(g);

if nargout==0
    x = (length(g)-1)/2;
    %color = random_color(1);
    color = 'b';
    plot(-x:x, g, 'Color', color);
    hold on
    plot(linspace(-x,x,50), myGaussian(sigma, linspace(-x,x,50)), '--', 'Color', color);
end

return




