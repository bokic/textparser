window.BENCHMARK_DATA = {
  "lastUpdate": 1787325726583,
  "repoUrl": "https://github.com/bokic/textparser",
  "entries": {
    "textparser SQLite C/H Parse Benchmark": [
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "4bef6586da8bd00042d79767171f165a1e57be11",
          "message": "Update benchmark workflow.",
          "timestamp": "2026-08-14T12:38:39+02:00",
          "tree_id": "d7a8eea7005000fb5de38be572ae4aebd94c05c4",
          "url": "https://github.com/bokic/textparser/commit/4bef6586da8bd00042d79767171f165a1e57be11"
        },
        "date": 1786704023704,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2159.072003111111,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2158.5843795555556 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2157.9941666666687,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2157.6402123333332 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.242206235784513,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.9680493887070427 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.001501666563742507,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0013749980852349876 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 55.185788444444164,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 55.18024022222158 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 55.23555100000029,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 55.229296333332435 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.14834944199037609,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.1437564471212843 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0026881819789477193,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0026052160436842806 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "8e870314398e740347b6ee27d7cdbc60acdba372",
          "message": "Update Python variant.",
          "timestamp": "2026-08-14T18:36:26+02:00",
          "tree_id": "9f293783cfa55cc5c8b90cb861e44eed6c5c8725",
          "url": "https://github.com/bokic/textparser/commit/8e870314398e740347b6ee27d7cdbc60acdba372"
        },
        "date": 1786725486446,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2986.068665444441,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2985.619074 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2988.2088876666635,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2987.7596206666663 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 5.422024666064785,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 5.522826408687949 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0018157736052120492,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0018498094605514128 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 77.27256433333372,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.25216366666663 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 77.28050366666632,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.26052166666737 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.07322020741758958,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.05617197348485712 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.000947557623450112,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0007271249220569681 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "b0c7dd24bb2c841dcddec94940242b5777434a51",
          "message": "Add Java implementation.",
          "timestamp": "2026-08-14T19:00:49+02:00",
          "tree_id": "cf40d81b696fa2f2a4161d050c9bd8c35305f53b",
          "url": "https://github.com/bokic/textparser/commit/b0c7dd24bb2c841dcddec94940242b5777434a51"
        },
        "date": 1786726953625,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2997.618347777783,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2997.221619111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2997.277549000008,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2996.7650106666665 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.9801131378525265,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.009959808209345 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0013277584655842173,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0013378923275612106 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 77.76245711110934,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.75476877777739 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 77.78653000002578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.77449999999912 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2536585037356118,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.25816496621829604 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003261966161552442,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0033202460797758884 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "7882dc4cafa4354c6eb99e0f1101bbfcd7588698",
          "message": "Add WebAssembly implementation.",
          "timestamp": "2026-08-14T20:41:12+02:00",
          "tree_id": "c7381af931976ea4d1cf4ad69705b2fd9d17ddd8",
          "url": "https://github.com/bokic/textparser/commit/7882dc4cafa4354c6eb99e0f1101bbfcd7588698"
        },
        "date": 1786732964821,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2529.175467555556,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2528.864329888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2530.434223999999,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2529.7492333333334 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 8.792062969762128,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.692574228803492 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003476256622977465,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0034373430500264984 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 65.65701666666573,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 65.65510366666692 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 65.64209833333241,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 65.64314033333336 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5213812636495828,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5144373391365111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007940983159447903,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00783545086987184 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "225b1866353d0fc60f1d44b004a630cd6dffb794",
          "message": "Update ecintella test project to use incremental parser.",
          "timestamp": "2026-08-14T21:40:46+02:00",
          "tree_id": "5548a0e8b2403c196122c52ace8a127d7b0bb067",
          "url": "https://github.com/bokic/textparser/commit/225b1866353d0fc60f1d44b004a630cd6dffb794"
        },
        "date": 1786736549444,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2688.0572382222217,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2687.556268888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2684.384551666663,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2683.994314000001 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 8.2265990704354,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.005577215980239 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0030604255569632727,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002978757062187937 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 69.69055733333738,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.6846036666668 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 69.63463533333918,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.6281186666674 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2224205827251771,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.22724630911347998 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003191545472384669,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003261069119378257 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "fdcf7372312db4c50a2b2dddd6f4ea47b12ef578",
          "message": "Check if ccontext_create() fails.",
          "timestamp": "2026-08-14T22:36:27+02:00",
          "tree_id": "0fdb2102430f88018578885b4d452e25bbb6a593",
          "url": "https://github.com/bokic/textparser/commit/fdcf7372312db4c50a2b2dddd6f4ea47b12ef578"
        },
        "date": 1786741004065,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2890.495099444444,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2889.989317666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2891.1656026666606,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2890.6583069999997 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.1137370242745575,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.9140066947852645 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0007312716166447816,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0006622885015819381 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 75.25297655555757,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.24218888888849 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 75.18075033333578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.17604499999919 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2202834761132444,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.2123729275683972 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002927239375715778,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0028225245796877625 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "673239793845eacb636c67b5855c94fcdbd6d59d",
          "message": "Document API functions.",
          "timestamp": "2026-08-15T10:25:59+02:00",
          "tree_id": "470920f5d6e73ea629dcaf316052044010352586",
          "url": "https://github.com/bokic/textparser/commit/673239793845eacb636c67b5855c94fcdbd6d59d"
        },
        "date": 1786782473932,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2915.1005441111106,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2914.5184111111107 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2912.9561653333367,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2912.4520406666657 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 7.113260091723409,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 7.046162606741102 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002440142281230449,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0024176078558566633 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 75.86503044444252,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.84209666666631 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 75.93711399999847,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.90526200000032 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.22472985201128773,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.22401854537171464 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0029622324105684225,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0029537493716227125 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "1b9f52cfb77426d02d97e4af79d9055a0736ca61",
          "message": "Add support for C++.",
          "timestamp": "2026-08-15T10:50:04+02:00",
          "tree_id": "367ef8dfc454a223d7dfaa74d406bfc0d87b7035",
          "url": "https://github.com/bokic/textparser/commit/1b9f52cfb77426d02d97e4af79d9055a0736ca61"
        },
        "date": 1786784032944,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2929.0692875555555,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2928.5008614444446 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2928.7004860000015,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2928.104403 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.044525999972737,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.0230834872712868 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0003566067912461171,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00034935399908561556 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 76.1633715555566,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.14332799999977 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 76.0978630000011,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.06655633333285 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.15591521700392616,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.17288069768617695 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0020471154810970437,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002270464165766139 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "80aa56386b1c7921f6ee0963560ebb9726ea57de",
          "message": "Implement error codes/string.",
          "timestamp": "2026-08-15T16:59:28+02:00",
          "tree_id": "651677fe15e1e782fde2a2c8b6a8d2124267cede",
          "url": "https://github.com/bokic/textparser/commit/80aa56386b1c7921f6ee0963560ebb9726ea57de"
        },
        "date": 1786806071502,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2724.3841629999965,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2723.883919444444 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2722.790863999999,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2722.2872826666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 6.9638883082967284,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 6.882191280961666 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0025561330163615175,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002526609607638984 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 70.51427955555785,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 70.50837377777775 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 70.40286900000108,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 70.40215066666633 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.35292911420826867,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3557313258082289 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005005072964408543,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005045235150783543 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "00a0f2229bb6510eff6bb70f7c481ba9b2367291",
          "message": "More work on imcremental parser.",
          "timestamp": "2026-08-17T23:18:45+02:00",
          "tree_id": "b229ec3613da0463ca756e94f59a3905084eae7d",
          "url": "https://github.com/bokic/textparser/commit/00a0f2229bb6510eff6bb70f7c481ba9b2367291"
        },
        "date": 1787001634081,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2116.564842666667,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2116.257029111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2114.4996136666664,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2114.210172333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 6.068104486664254,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 6.105098101772548 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002866958934751571,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0028848566208126737 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 54.98158477777634,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 54.97759066666674 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 54.851309000000015,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 54.84642433333301 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.3281762286997174,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.32532298880240657 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005968839021758552,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005917374422150732 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "b17353aa8c610a30cb7fdb21ade40ee300648459",
          "message": "Add FUTURE_WORK.md",
          "timestamp": "2026-08-18T14:05:27+02:00",
          "tree_id": "8517a068ed22d5a7a93343d45c68d3e214a50b09",
          "url": "https://github.com/bokic/textparser/commit/b17353aa8c610a30cb7fdb21ade40ee300648459"
        },
        "date": 1787054826840,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2642.9564561111106,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2642.656113888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2643.2107193333345,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2642.899526666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 9.789127738177031,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 9.75029720130587 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003703855095887926,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003689582291869786 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 68.92696800000111,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 68.92405755555546 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 68.87438566666522,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 68.86827433333309 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.28152682095333603,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.28404484620419734 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004084421948653414,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004121127749556041 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "e5bc82de1607f285be205d0c2ac9ba75b01dc6b2",
          "message": "Remove position from token struct.",
          "timestamp": "2026-08-21T01:10:05+02:00",
          "tree_id": "27ba2c98f6fbed2979402666bc8209f0605884a9",
          "url": "https://github.com/bokic/textparser/commit/e5bc82de1607f285be205d0c2ac9ba75b01dc6b2"
        },
        "date": 1787267510115,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2691.105822777776,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2690.724533333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2692.4058563333233,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2692.012393 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.9618648138968324,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.9471966956981333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.001472206994003443,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001466964249516933 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 61.35778811110665,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 61.253555333333765 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 61.22724466666796,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 61.2042456666669 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.335926074089234,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.1955364062720546 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.021772722179458942,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.019517828798117975 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "51e782b8784aee15cff6e72474404099f95be651",
          "message": "Update incremental parsing algorithm",
          "timestamp": "2026-08-21T01:41:56+02:00",
          "tree_id": "a9bf680dcf945d7fcadac4fd7c91e18ae92ef858",
          "url": "https://github.com/bokic/textparser/commit/51e782b8784aee15cff6e72474404099f95be651"
        },
        "date": 1787269422102,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2704.5186478888877,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2704.0368563333327 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2698.5129403333303,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2697.9955639999994 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 12.907461369971676,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 12.908071213252198 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0047725540291789345,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004773629909303643 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 60.64040911111116,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 60.6196991111112 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 60.55508600000318,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 60.545927666666444 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.20097918244244647,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.20591527935728704 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0033142781420585927,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003396837700890932 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "a91774ef68dfa46fca3fc153d1b88c61a7781517",
          "message": "Performance optimizations.",
          "timestamp": "2026-08-21T06:17:31+02:00",
          "tree_id": "6cb947b01c01ef27309bbec116558e6af90f8e76",
          "url": "https://github.com/bokic/textparser/commit/a91774ef68dfa46fca3fc153d1b88c61a7781517"
        },
        "date": 1787285946509,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1555.7098695555521,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1555.3424804444446 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1554.4302896666597,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1554.2191130000003 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.269265032066419,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.991823452905652 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0014586685322724871,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001280633351142368 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 42.03740788888884,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 42.03406277777786 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 41.93688533333292,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 41.9372220000002 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.32773118548481084,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3259834616834619 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007796179687174181,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007755221364321687 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "ae97f90328a27a2c9e3eef708e596b66ecdb6ed6",
          "message": "Update FUTURE_WORK.md",
          "timestamp": "2026-08-21T08:39:11+02:00",
          "tree_id": "e5630946a67f1a6397a8d39395d2a82053149fec",
          "url": "https://github.com/bokic/textparser/commit/ae97f90328a27a2c9e3eef708e596b66ecdb6ed6"
        },
        "date": 1787294438322,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1561.4868133333332,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1560.6299569999999 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1557.389377333332,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1556.997939333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 9.10192502333642,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.273887459657848 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005829011776222677,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005301633114593513 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 42.022003111112674,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 42.01960888888894 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 41.99110566667249,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 41.99058600000013 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.06533092061223085,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.06191827156234022 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0015546836365578716,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001473556589402549 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "ec8eb46194826da3e0f1d805d4003645fe05e2fb",
          "message": "Implement AST Post-Processing with Pratt Parsing.",
          "timestamp": "2026-08-21T08:57:35+02:00",
          "tree_id": "fd2f998cd3b5610585a50ddc5eb44ba1cb64050e",
          "url": "https://github.com/bokic/textparser/commit/ec8eb46194826da3e0f1d805d4003645fe05e2fb"
        },
        "date": 1787295556093,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1252.5638258888894,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1252.424081111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1246.8936403333307,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1246.7601260000001 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 15.246085734487401,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 15.233196604714188 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.012171903275003112,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0121629700629836 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 34.900370111110924,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 34.89435633333334 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 35.17052466666826,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 35.17132300000014 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.678309289402319,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.6779306192472742 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01943559014539997,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.01942808781945266 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "9c537f2f0d7b981f1280c543b7339918a5049383",
          "message": "Update tests.",
          "timestamp": "2026-08-21T09:20:39+02:00",
          "tree_id": "f6a7d0d0bc279bc9927d4d7320ca1bd83f422efe",
          "url": "https://github.com/bokic/textparser/commit/9c537f2f0d7b981f1280c543b7339918a5049383"
        },
        "date": 1787296953302,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1669.5456188888882,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1669.3176610000003 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1670.5900969999968,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1670.2946103333334 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.1995167877853983,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.153751344119691 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0019163997386994031,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0018892457785598727 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 45.217800888888995,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.204880555555604 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 45.20567166666467,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.20591033333332 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.41437132773230867,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3946130077378082 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.009163898278700431,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.008729433700258077 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "dbfd9858519b06c8805c9e0d81e129e19850a8f2",
          "message": "Enable Windows CI unittests.",
          "timestamp": "2026-08-21T09:33:00+02:00",
          "tree_id": "ca60bed5e279fd35322f824ba2b4437983bf6089",
          "url": "https://github.com/bokic/textparser/commit/dbfd9858519b06c8805c9e0d81e129e19850a8f2"
        },
        "date": 1787297726291,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1647.7021675555552,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1647.4923282222219 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1649.2013810000024,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1649.015185 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.1578957677375996,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.148770696344054 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0019165452530917578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0019112505972891743 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 44.898403222223095,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 44.88456755555583 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 44.78775900000187,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 44.774897333333875 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4525333410606837,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4477860941393545 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01007905200594519,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.009976393190936008 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "c04bc93bb678990d0308a7ed159520e94157aa0b",
          "message": "Implement Contextual Rule Disambiguation.",
          "timestamp": "2026-08-21T10:01:50+02:00",
          "tree_id": "5e077313f1f899869d9bbbe862f5c59b97d93c23",
          "url": "https://github.com/bokic/textparser/commit/c04bc93bb678990d0308a7ed159520e94157aa0b"
        },
        "date": 1787299424264,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1159.8325639999985,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1159.626543 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1132.3592810000018,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1132.1562356666668 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 47.90216201602734,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 47.95659530853292 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.04130092868820969,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.041355206637877825 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 31.53277800000056,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 31.525704333333433 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 31.497900000005075,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 31.493321333333196 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4409091806051764,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.44343055076487436 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.013982566984905949,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.014065682595900544 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "14deec7222a008d1732fa796dc455803dff4b23e",
          "message": "Implement Flat Token Range Struct.",
          "timestamp": "2026-08-21T15:34:18+02:00",
          "tree_id": "7ddbdfd6a1cd610c41b02473ab19dc29953caeeb",
          "url": "https://github.com/bokic/textparser/commit/14deec7222a008d1732fa796dc455803dff4b23e"
        },
        "date": 1787319346518,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1807.0583084444454,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1806.760609111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1808.5740603333372,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1808.2558593333335 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.7011312880141176,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.698756717116748 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0014947670893582338,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0014936991118289218 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 48.41145855555359,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.40624000000006 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 48.40159233333452,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.39633033333376 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.037288153234434636,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.03195803557055218 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0007702340385312988,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.000660204873804537 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "e02b5818f9b2aea1eb81efb84f9ce49d97d50e4a",
          "message": "Implement Native C Regex Bypass",
          "timestamp": "2026-08-21T17:12:43+02:00",
          "tree_id": "fa27a50362dc754371181b8411665f85007a3fd7",
          "url": "https://github.com/bokic/textparser/commit/e02b5818f9b2aea1eb81efb84f9ce49d97d50e4a"
        },
        "date": 1787325246145,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 770.5605186666686,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 770.4201845555555 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 771.2928423333333,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 771.150411 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 5.69666631526535,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 5.649499459419457 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007392886317511463,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007333010703345699 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 20.640517555555167,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.64119111111109 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 20.408064666668224,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.408726000000026 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5117638933541064,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5116794920844469 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.024794140552758127,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.024789242506892507 ms\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "3b03cb8d1708d98f9632f6282d477631272b23b3",
          "message": "Fix windows build.",
          "timestamp": "2026-08-21T17:20:51+02:00",
          "tree_id": "ab4fb29d538dafc2d33f37347a415242d6a06595",
          "url": "https://github.com/bokic/textparser/commit/3b03cb8d1708d98f9632f6282d477631272b23b3"
        },
        "date": 1787325725883,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 615.6268526666667,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 615.5540145555555 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 618.3027959999995,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 618.2417409999999 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.689784427153515,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.68927746544149 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007617901017213775,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007617978852476917 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 15.426112222221553,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 15.425419777777746 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 15.428784000003285,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 15.42913866666673 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.0696241251509172,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.07131875711847196 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004513394181757772,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004623456485846537 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}