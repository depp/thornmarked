def n64_repositories(workspace_dir):
    native.new_local_repository(
        name = "n64sdk",
        build_file = "//n64:n64sdk.bazel",
        path = workspace_dir + "/sdk/ultra",
    )
