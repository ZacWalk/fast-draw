@{
    schema = 1
    project = @{
        name = 'fast-draw'
        type = 'gui'
        'default-target' = 'app'
    }
    dependencies = @{ owner = 'dd' }
    build = @{
        'x64-windows' = @{
            debug = 'x64-debug'
            release = 'x64-release'
        }
    }
    targets = @(
        @{
            id = 'app'
            kind = 'gui'
            'cmake-target' = 'fast-draw'
            'test-label' = 'fast-draw'
            'debug-path' = 'build/x64-debug/fast-draw{exe}'
            'release-path' = 'build/x64-release/fast-draw{exe}'
            platforms = @('x64-windows')
        }
    )
}
