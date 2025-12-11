def build(bld):
    bld.build_a_script('dce', needed = ['core', 'network', 'internet', 'dce', 'dce-quagga', 'mobility', 'netanim', 'point-to-point', 'applications'],
        target='bin/satnet-rfp',
        source=['examples/satnet-rfp-main.cc'],
        includes=['.', 'src']
    )
