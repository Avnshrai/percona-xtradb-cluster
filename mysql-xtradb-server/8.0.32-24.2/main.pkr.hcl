packer {
  required_plugins {
    docker = {
      version = ">= 0.0.7"
      source  = "github.com/hashicorp/docker"
    }
  }
}

source "docker" "mysql-xtradb-server" {
  image  = "ubuntu:jammy"  # Adjust the base image according to your requirements
  commit = true
  volumes = {
    "/var/run/docker.sock" = "/var/run/docker.sock"
  }
}
variable "docker_username" {
  type    = string
  default = ""
}
variable "docker_password" {
  type    = string
  default = ""
}
variable "tag" {
  type    = string
  default = ""
}
variable "branch" {
  type    = string
  default = ""
}
variable "git_token" {
  type    = string
  default = ""
}

build {
  name = "mysql-xtradb-operator-Image"
  sources = [
    "source.docker.mysql-xtradb-server"
  ]


  provisioner "shell" {
    inline = [
      "apt-get update",
      "DEBIAN_FRONTEND=noninteractive apt-get install -y curl wget jq ca-certificates git gnupg lsb-release sudo software-properties-common",
      "curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -",
      "sudo add-apt-repository \"deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable\"",
      "sudo apt-get update",
      "sudo apt-get install -y docker.io",
      "sudo apt install docker-buildx",
      "sudo apt-get update",
      "docker login -u ${var.docker_username} -p ${var.docker_password}",
    ]
    environment_vars = [
      "DOCKER_DEFAULT_PLATFORM=linux/amd64",
      "IMAGE=coredgeio/mysql-xtradb-server:${var.tag}",
    ]
  }
  post-processor "docker-tag" {
    repository = "coredgeio/mysql-xtradb-server"  #Adjust repository name as needed
    tags       = ["latest"]
  }
}
