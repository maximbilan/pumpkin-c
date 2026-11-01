/* pumpkin.readable.c: the same program as pumpkin.c, unpacked and commented.
 * It draws exactly the same frames.
 *
 * Every character cell on the screen casts a ray. The ray is marched through
 * a signed distance field of a ribbed pumpkin with a stem. Where it lands on
 * the carved face, it carries on inside the pumpkin to the back wall, which is
 * lit by a flickering candle. Brightness then picks a character from a ramp
 * and a 24-bit color.
 *
 *   cc pumpkin.readable.c && ./a.out        (add -lm on Linux)
 */
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>

int cols = 80, rows = 24;
int hit_stem;         /* set by pumpkin(): 1 when the stem was the closer part */
float squash = 1.342; /* the body is this much wider than it is tall */
struct winsize ws;

/* Distance from (x, y, z) to the pumpkin, in the pumpkin's own coordinates:
   y up, face toward -z. It's scaled by .6 so the ribs never make a ray
   overshoot the surface. */
float pumpkin(float x, float y, float z) {
    float r = sqrt(x * x + z * z),
          body = sqrt(r * r + y * y * squash * squash) - 1
                 + .1 * (1 - sqrt(fabs(sin(5 * atan2(z, x))))) * fmin(r, 1) /* ten ribs */
                 + (y > 0) * .12 * exp(-9 * r * r),                         /* dip around the stem */
          bend = x - .3 * (y - .6) * (y - .6),                              /* the stem leans */
          stem = fmax(sqrt(bend * bend + z * z) - .1 + .05 * (y - .6), fabs(y - .74) - .24);
    hit_stem = stem < body;
    return fmin(body, stem) * .6;
}

/* Is the point (x, y) on the front of the pumpkin cut out? Two triangle eyes,
   a triangle nose, and a grin with two teeth on top and one at the bottom. */
int carved(float x, float y) {
    float ax = fabs(x), s = x / .6, k = 1 - s * s,
          top = -.1 - .15 * k, bottom = -.14 - .31 * k;
    return (y > .04 && y < .4 && fabs(ax - .35) < (.4 - y) * .62)       /* eyes */
        || (y > -.06 && y < .06 && ax < (.06 - y) * .55)                /* nose */
        || (k > 0 && y < top && y > bottom                              /* grin */
            && !(fabs(ax - .22) < .06 && y > top - .08)
            && !(ax < .06 && y < bottom + .07));
}

/* Ctrl-C: reset the color, show the cursor again, clear the screen. */
void quit(int sig) {
    write(1, "\33[m\33[?25h\33[2J\33[H", 19);
    _exit(0);
}

int main() {
    signal(SIGINT, quit);
    ioctl(1, TIOCGWINSZ, &ws);
    if (ws.ws_col) cols = ws.ws_col, rows = ws.ws_row;
    /* Cells are about twice as tall as they are wide; fit the pumpkin to
       whichever of width and height runs out first. */
    float zoom = fmin(cols, 2.2 * rows) * 1.45;
    printf("\33[2J\33[?25l"); /* clear the screen, hide the cursor */

    for (int frame = 0;; frame++) {
        float secs = frame / 60.;

        /* Spin around the vertical axis. The sine term slows the turn while
           the face is toward you and hurries it past the back. */
        float angle = secs * .8;
        angle -= .6 * sin(angle);
        float sn = sin(angle), cs = cos(angle);

        float flicker = .85 + .1 * sin(secs * 9) + .07 * sin(secs * 23 + 1),
              /* Moonlight from the upper left (y = .5), turned into pumpkin space. */
              light_x = -.7 * cs - .5 * sn, light_z = .7 * sn - .5 * cs;

        printf("\33[H"); /* draw over the last frame */
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                float u = (col - cols / 2.) / zoom, v = (rows / 2. - row) * 2 / zoom - .07,
                      dx = cs * u + sn, dy = v, dz = cs - sn * u,
                      len = sqrt(dx * dx + dy * dy + dz * dz),
                      t = 0, dist = 1, x, y, z;
                dx /= len; dy /= len; dz /= len;

                /* March until the ray touches the pumpkin or gives up. Rather
                   than the pumpkin turning, the camera circles it at 3.6. */
                for (int step = 0; step < 90 && dist > .002 && t < 7; step++)
                    x = -3.6 * sn + t * dx, y = .25 + t * dy, z = -3.6 * cs + t * dz,
                    t += dist = pumpkin(x, y, z);

                int shade = -1, red = 0, green = 0, blue = 0, stem = hit_stem;
                if (dist < .01) {
                    if (!stem && z < 0 && carved(x, y)) {
                        /* Through the face: find where the ray meets the
                           inside wall (an ellipsoid a bit smaller than the
                           body) and light it from the candle. */
                        float ey = dy * squash, qy = y * squash,
                              a = dx * dx + ey * ey + dz * dz,
                              b = x * dx + qy * ey + z * dz,
                              back = (-b + sqrt(fmax(0, b * b - a * (x * x + qy * qy + z * z - .8)))) / a,
                              wx = x + back * dx, wy = y + back * dy, wz = z + back * dz,
                              /* Squared distance from that wall point to the candle. */
                              candle = wx * wx + (wy + .42) * (wy + .42) + wz * wz,
                              /* How squarely the wall faces the candle. */
                              facing = (wx * wx + wy * squash * squash * (wy + .42) + wz * wz)
                                     / sqrt(candle * (wx * wx + wy * wy * squash * squash * squash * squash + wz * wz)),
                              /* The point on the ray closest to the flame. */
                              closest = fmin(fmax(-x * dx - (.42 + y) * dy - z * dz, 0), back),
                              fx = x + closest * dx, fy = y + closest * dy + .42, fz = z + closest * dz,
                              glow = .55 + .45 * fmax(facing, 0) * .3 / candle;
                        glow = fmin(flicker * (glow + exp(-(fx * fx + fy * fy * .4 + fz * fz) * 40)), 1);
                        shade = 9 + 2.99 * glow;
                        red = 255;
                        green = 165 + 90 * glow;
                        blue = 50 + 170 * glow * glow;
                    } else {
                        /* The skin: surface normal from the distance field. */
                        float e = .01, here = pumpkin(x, y, z),
                              nx = pumpkin(x + e, y, z) - here,
                              ny = pumpkin(x, y + e, z) - here,
                              nz = pumpkin(x, y, z + e) - here,
                              nlen = sqrt(nx * nx + ny * ny + nz * nz);
                        nx /= nlen; ny /= nlen; nz /= nlen;
                        float diffuse = fmax(0, nx * light_x + ny * .5 + nz * light_z),
                              hx = light_x - dx, hy = .5 - dy, hz = light_z - dz,
                              hlen = sqrt(hx * hx + hy * hy + hz * hz),
                              shine = pow(fmax(0, (nx * hx + ny * hy + nz * hz) / hlen), 30),
                              /* Darken the grooves between ribs. */
                              occlusion = fmin(fmax(pumpkin(x + nx * .1, y + ny * .1, z + nz * .1) / .06, 0), 1),
                              lum = fmin(.05 + .75 * diffuse * occlusion + .35 * shine, 1);
                        shade = lum * 9.99;
                        if (stem)
                            red = 110 * (.3 + lum), green = 140 * (.3 + lum), blue = 40 * (.3 + lum);
                        else
                            red = 235 * (.2 + .8 * lum), green = 95 * (.15 + .85 * lum) + 50 * shine, blue = 10 + 60 * shine;
                    }
                }
                if (shade < 0) putchar(' ');
                else printf("\33[38;2;%d;%d;%dm%c", red, green, blue, ".,-~:;=!*#$@"[shade]);
            }
            if (row < rows - 1) putchar('\n');
        }
        fflush(stdout);
        usleep(16000);
    }
}
