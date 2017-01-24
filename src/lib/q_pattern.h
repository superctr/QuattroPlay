#ifndef Q_PATTERN_H_INCLUDED
#define Q_PATTERN_H_INCLUDED

struct QP_Pattern {
    int pat[32][8];
    int len;
};
void QP_PatternGenerate(int TrackNo,struct QP_Pattern* P);

#endif // Q_PATTERN_H_INCLUDED
