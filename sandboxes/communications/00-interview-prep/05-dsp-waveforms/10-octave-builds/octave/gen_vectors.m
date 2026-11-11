% gen_vectors.m -- Octave reference model + test-vector generator for Pack 10.
%
%   Run (from anywhere):   octave --no-gui octave/gen_vectors.m
%
% Writes plain-text vectors (one number per line, %.17g) into ../vectors relative to this file.
% Core Octave only (no packages).  The inputs are generated deterministically (Park-Miller LCG),
% so every run gives the same files, on every platform.  Synthetic signals only.
%
% NOT RUN by the pack author (no Octave in the authoring environment): if a function name differs on your
% Octave version, fix it -- the C++ tests simply skip a comparison whose vector file is missing.
1;   % marks this as a script file so the functions below can be defined first

function write_vec(outdir, name, v)
  fid = fopen(fullfile(outdir, name), 'w');
  if fid < 0
    error('gen_vectors: cannot open %s for writing', name);
  end
  fprintf(fid, '%.17g\n', v(:));
  fclose(fid);
end

function u = lcg_uniform(n, seed)
  % Park-Miller "minimal standard": x <- 16807 x mod (2^31 - 1).
  % 16807 * (2^31 - 2) < 2^46, so every product is exact in a double: identical on all machines.
  x = seed;
  u = zeros(1, n);
  for k = 1:n
    x = mod(16807 * x, 2147483647);
    u(k) = x / 2147483647;          % in (0,1)
  end
end

function y = q15_fir_ref(x, h)
  % y[n] = sat16( floor((sum_k h[k] x[n-k] + 2^14) / 2^15) ); all values are integers held exactly in doubles
  % (|products| <= 2^30, sums of a few terms << 2^53), and division by 2^15 is exact, so this is bit-exact.
  n = numel(x);
  y = zeros(1, n);
  for i = 1:n
    acc = 0;
    for k = 1:min(numel(h), i)
      acc = acc + h(k) * x(i - k + 1);
    end
    v = floor((acc + 16384) / 32768);
    y(i) = min(max(v, -32768), 32767);
  end
end

here = fileparts(mfilename('fullpath'));
if isempty(here)
  here = '.';
end
outdir = fullfile(here, '..', 'vectors');
if ~exist(outdir, 'dir')
  mkdir(outdir);
end

u = lcg_uniform(1300, 20240229);    % one stream, disjoint slices below

% ---------------------------------------------------------------- 1. FFT (N = 64, complex input)
xr = u(1:64) - 0.5;
xi = u(65:128) - 0.5;
x  = xr + 1j * xi;
X  = fft(x);
write_vec(outdir, 'fft_in_re.txt',  real(x));
write_vec(outdir, 'fft_in_im.txt',  imag(x));
write_vec(outdir, 'fft_out_re.txt', real(X));
write_vec(outdir, 'fft_out_im.txt', imag(X));

% ---------------------------------------------------------------- 2. FIR (windowed sinc, 21 taps, fc = 0.1)
N  = 21;
fc = 0.1;
m  = (0:N-1) - (N-1)/2;
t  = 2 * fc * m;
sn = ones(size(t));
nz = (t ~= 0);
sn(nz) = sin(pi * t(nz)) ./ (pi * t(nz));          % sinc without the signal package
w  = 0.54 - 0.46 * cos(2 * pi * (0:N-1) / (N-1));  % symmetric Hamming
h  = 2 * fc * sn .* w;
xin = 2 * (u(129:328) - 0.5);                      % 200 samples in (-1, 1)
y   = filter(h, 1, xin);                           % zero initial state, same length as xin
write_vec(outdir, 'fir_taps.txt', h);
write_vec(outdir, 'fir_in.txt',   xin);
write_vec(outdir, 'fir_out.txt',  y);

% ---------------------------------------------------------------- 3. 4FSK modulator (levels exact, waveform float)
sps = 10;  dev = 600;  fs = 48000;                 % 4800 sym/s, 600 Hz deviation unit (VERIFY P25 C4FM)
dibits = floor(4 * u(329:368));                    % 40 dibits, values 0..3
lvmap  = [1 3 -1 -3];                              % dibit 00 -> +1, 01 -> +3, 10 -> -1, 11 -> -3
lev    = lvmap(dibits + 1);
f      = kron(lev, ones(1, sps)) * dev;            % Hz, one value per sample
dphi   = 2 * pi * f / fs;
phi    = [0, cumsum(dphi(1:end-1))];               % phi[0] = 0, phi[n+1] = phi[n] + dphi[n]
z      = exp(1j * phi);
write_vec(outdir, 'fsk4_dibits.txt', dibits);
write_vec(outdir, 'fsk4_levels.txt', lev);
write_vec(outdir, 'fsk4_out_re.txt', real(z));
write_vec(outdir, 'fsk4_out_im.txt', imag(z));

% ---------------------------------------------------------------- 4. 48-bit sync correlator (hard decision, exact)
wordbits = dec2bin(hex2dec('5575F5FF77FF'), 48) - '0';   % P25 frame-sync pattern (VERIFY against TIA-102.BAAA)
rx = floor(2 * u(369:768));                              % 400 random bits
at = 101;                                                % 1-based start => 0-based lag 100
emb = wordbits;
emb(5)  = 1 - emb(5);                                    % two bit errors inside the embedded word
emb(30) = 1 - emb(30);
rx(at:at+47) = emb;
nl = numel(rx) - 48 + 1;
d  = zeros(1, nl);
for l = 1:nl
  d(l) = sum(xor(rx(l:l+47), wordbits));
end
write_vec(outdir, 'sync_word.txt', wordbits);
write_vec(outdir, 'sync_rx.txt',   rx);
write_vec(outdir, 'sync_dist.txt', d);

% ---------------------------------------------------------------- 5. CFO estimate and correction
n  = 0:127;
f0 = 0.0123;  theta = 0.7;
kn = 2 * floor(2 * u(769:896)) - 1;                      % known +-1 sequence
nz_r = 0.02 * (u(897:1024)  - 0.5);
nz_i = 0.02 * (u(1025:1152) - 0.5);
rxc  = kn .* exp(1j * (2 * pi * f0 * n + theta)) + (nz_r + 1j * nz_i);
r    = rxc .* kn;
fhat = angle(sum(r(2:end) .* conj(r(1:end-1)))) / (2 * pi);
cout = rxc .* exp(-1j * 2 * pi * fhat * n);
write_vec(outdir, 'cfo_known.txt',  kn);
write_vec(outdir, 'cfo_in_re.txt',  real(rxc));
write_vec(outdir, 'cfo_in_im.txt',  imag(rxc));
write_vec(outdir, 'cfo_est.txt',    fhat);
write_vec(outdir, 'cfo_out_re.txt', real(cout));
write_vec(outdir, 'cfo_out_im.txt', imag(cout));

% ---------------------------------------------------------------- 6. Q15 FIR (bit-exact, includes saturation)
xq = round(32767 * (2 * u(1153:1272) - 1));
xq(1:4) = [32767 32767 -32768 -32768];                   % forces the 4th output to saturate
hq = [12000 -8000 20000 20000 -8000 12000];              % sum = 48000 / 32768 = 1.46 (> 1)
yq = q15_fir_ref(xq, hq);
write_vec(outdir, 'q15_x.txt', xq);
write_vec(outdir, 'q15_h.txt', hq);
write_vec(outdir, 'q15_y.txt', yq);

% ---------------------------------------------------------------- 7. QPSK/BPSK BER theory (per bit): Q(sqrt(2 Eb/N0))
ebn0 = [0 2 4 6 8];
ber  = 0.5 * erfc(sqrt(10 .^ (ebn0 / 10)));
fid = fopen(fullfile(outdir, 'ber_theory.txt'), 'w');
fprintf(fid, '%.17g %.17g\n', [ebn0; ber]);              % columns of the 2xN matrix: "ebn0 ber" per line
fclose(fid);

fprintf('gen_vectors: wrote vectors to %s\n', outdir);
fprintf('  fft N=64, fir N=21 x 200, 4fsk %d dibits, sync 48 in 400 bits, cfo est = %.12g, q15 %d samples\n', ...
       numel(dibits), fhat, numel(xq));
